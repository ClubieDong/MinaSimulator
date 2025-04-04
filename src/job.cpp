#include "job.hpp"
#include "data.hpp"
#include "utils/trace.hpp"
#include <cassert>

double Job::CalcStepDuration(bool useSharp) const {
    double stepDuration = 0.0;
    for (const auto &opGroup : CommOpGroups) {
        double groupDuration = 0.0;
        for (const auto &op : opGroup.CommOps) {
            auto opDuration = CalcTransmissionDuration(op.OpType, op.MessageSize, useSharp, HostCount);
            groupDuration = std::max(groupDuration, op.StartTimeInGroup) + opDuration;
        }
        groupDuration = std::max(groupDuration, opGroup.SyncTime);
        stepDuration += groupDuration;
    }
    return stepDuration;
}

Job::Job(const char *modelName, unsigned int hostCount, std::optional<unsigned int> stepCount)
    : ID(m_NextID++), ModelName(modelName), HostCount(hostCount), StepCount(stepCount),
      CommOpGroups(ModelInfoProvider::GetModelInfo(ModelName)) {
    StepDurationWithSharp = CalcStepDuration(true);
    StepDurationWithoutSharp = CalcStepDuration(false);
}

double Job::GetNextEvent(double now) const {
    if (!m_IsStarted)
        return now;
    assert(!m_IsFinished);
    assert(!StepCount || m_CurrentStepIdx < *StepCount);
    assert(m_CurrentGroupIdx < CommOpGroups.size());
    const auto &opGroup = CommOpGroups[m_CurrentGroupIdx];
    if (m_CurrentOpIdx >= opGroup.CommOps.size()) {
        assert(!m_IsRunning);
        return std::max(now, m_CurrentGroupStartTime + opGroup.SyncTime);
    }
    const auto &op = opGroup.CommOps[m_CurrentOpIdx];
    if (m_IsRunning)
        return m_CurrentTransmissionStartTime + m_CurrentTransmissionDuration;
    return std::max(now, std::max(m_WaitingUntilTime, m_CurrentGroupStartTime + op.StartTimeInGroup));
}

bool Job::RunNextEvent(double now) {
    if (!m_IsStarted) {
        Trace.RecordBeginJob(now, *this);
        Trace.RecordBeginStep(now, *this);
        Trace.RecordBeginGroup(now, *this);
        m_IsStarted = true;
        m_StartTime = now;
        m_CurrentGroupStartTime = now;
        return false;
    }
    assert(!m_IsFinished);
    assert(!StepCount || m_CurrentStepIdx < *StepCount);
    assert(m_CurrentGroupIdx < CommOpGroups.size());
    const auto &opGroup = CommOpGroups[m_CurrentGroupIdx];
    if (m_CurrentOpIdx >= opGroup.CommOps.size()) {
        assert(!m_IsRunning);
        assert(now >= m_CurrentGroupStartTime + opGroup.SyncTime);
        Trace.RecordEndGroup(now, *this);
        m_CurrentOpIdx = 0;
        ++m_CurrentGroupIdx;
        if (m_CurrentGroupIdx >= CommOpGroups.size()) {
            Trace.RecordEndStep(now, *this);
            m_CurrentGroupIdx = 0;
            ++m_CurrentStepIdx;
            if (StepCount && m_CurrentStepIdx >= *StepCount) {
                Trace.RecordEndJob(now, *this);
                m_IsFinished = true;
                m_FinishTime = now;
                return true;
            }
            Trace.RecordBeginStep(now, *this);
        }
        Trace.RecordBeginGroup(now, *this);
        m_CurrentGroupStartTime = now;
        return false;
    }
    const auto &op = opGroup.CommOps[m_CurrentOpIdx];
    if (m_IsRunning) {
        assert(now == m_CurrentTransmissionStartTime + m_CurrentTransmissionDuration);
        Trace.RecordEndTransmission(now, *this);
        m_IsRunning = false;
        m_CurrentOpTransmittedMessageSize += m_TransmittingMessageSize;
        assert(m_CurrentOpTransmittedMessageSize <= op.MessageSize);
        if (m_CurrentOpTransmittedMessageSize == op.MessageSize) {
            Trace.RecordEndCommOp(now, *this);
            m_CurrentOpTransmittedMessageSize = 0;
            ++m_CurrentOpIdx;
        }
        (m_IsUsingSharp ? m_DurationWithSharp : m_DurationWithoutSharp) += m_CurrentTransmissionDuration;
        m_AfterTransmissionCallback(*this, now);
        if (m_NextAggrTree) {
            if (*m_NextAggrTree != m_AggrTree)
                ++m_TreeMigrationCount;
            m_AggrTree = std::move(*m_NextAggrTree);
            m_NextAggrTree = std::nullopt;
        }
        return false;
    }
    if (m_CurrentOpTransmittedMessageSize == 0 && now != m_WaitingUntilTime)
        Trace.RecordBeginCommOp(now, *this);
    auto scheduleRes = m_BeforeTransmissionCallback(*this, now);
    if (scheduleRes.InsertWaitingTime) {
        Trace.RecordBeginWaiting(now, *this);
        m_WaitingUntilTime = now + scheduleRes.WaitingTime;
        return false;
    }
    if (now == m_WaitingUntilTime) {
        Trace.RecordEndWaiting(now, *this);
        m_WaitingUntilTime = 0.0;
    }
    assert(!scheduleRes.UseSharp || m_AggrTree);
    m_IsRunning = true;
    m_IsUsingSharp = scheduleRes.UseSharp;
    m_TransmittingMessageSize = scheduleRes.MessageSize;
    if (m_TransmittingMessageSize == -1ull)
        m_TransmittingMessageSize = op.MessageSize - m_CurrentOpTransmittedMessageSize;
    assert(m_CurrentOpTransmittedMessageSize + m_TransmittingMessageSize <= op.MessageSize);
    m_CurrentTransmissionDuration =
        CalcTransmissionDuration(op.OpType, m_TransmittingMessageSize, m_IsUsingSharp, HostCount);
    m_CurrentTransmissionStartTime = now;
    Trace.RecordBeginTransmission(now, *this);
    return false;
}

std::optional<CommOpRunningInfo> Job::GetNextCommOpInfo(double now) const {
    if (m_IsFinished)
        return std::nullopt;
    assert(!StepCount || m_CurrentStepIdx < *StepCount);
    assert(m_CurrentGroupIdx < CommOpGroups.size());
    auto opTransmittedMessageSize = m_CurrentOpTransmittedMessageSize;
    auto opIdx = m_CurrentOpIdx, groupIdx = m_CurrentGroupIdx;
    auto groupStartTime = m_CurrentGroupStartTime;
    if (m_IsRunning) {
        now = m_CurrentTransmissionStartTime + m_CurrentTransmissionDuration;
        const auto &opGroup = CommOpGroups[groupIdx];
        assert(opIdx < opGroup.CommOps.size());
        const auto &op = opGroup.CommOps[opIdx];
        opTransmittedMessageSize += m_TransmittingMessageSize;
        assert(opTransmittedMessageSize <= op.MessageSize);
        if (opTransmittedMessageSize == op.MessageSize) {
            opTransmittedMessageSize = 0;
            ++opIdx;
        }
    }
    if (opIdx >= CommOpGroups[groupIdx].CommOps.size()) {
        now = std::max(now, groupStartTime + CommOpGroups[groupIdx].SyncTime);
        opIdx = 0;
        ++groupIdx;
        if (groupIdx >= CommOpGroups.size()) {
            groupIdx = 0;
            if (StepCount && m_CurrentStepIdx + 1 >= *StepCount)
                return std::nullopt;
        }
        groupStartTime = now;
    }
    const auto &op = CommOpGroups[groupIdx].CommOps[opIdx];
    auto startTime = std::max(now, std::max(m_WaitingUntilTime, groupStartTime + op.StartTimeInGroup));
    auto durationWithSharp =
        CalcTransmissionDuration(op.OpType, op.MessageSize - opTransmittedMessageSize, true, HostCount);
    auto durationWithoutSharp =
        CalcTransmissionDuration(op.OpType, op.MessageSize - opTransmittedMessageSize, false, HostCount);
    return CommOpRunningInfo{groupStartTime, startTime, durationWithSharp, durationWithoutSharp, groupIdx, opIdx};
}

double Job::GetNextCommOpPriority(const CommOpRunningInfo &commOpInfo) const {
    auto calcGroupFinishTime = [this, &commOpInfo](bool useSharpOnNext, bool useSharpOnRest) -> double {
        const auto &opGroup = CommOpGroups[commOpInfo.GroupIdx];
        auto now =
            commOpInfo.OpStartTime + (useSharpOnNext ? commOpInfo.DurationWithSharp : commOpInfo.DurationWithoutSharp);
        for (unsigned int opIdx = commOpInfo.OpIdx + 1; opIdx < opGroup.CommOps.size(); ++opIdx) {
            const auto &op = opGroup.CommOps[opIdx];
            auto duration = CalcTransmissionDuration(op.OpType, op.MessageSize, useSharpOnRest, HostCount);
            now = std::max(now, commOpInfo.GroupStartTime + op.StartTimeInGroup) + duration;
        }
        return std::max(now, commOpInfo.GroupStartTime + opGroup.SyncTime);
    };
    // TODO: should useSharpOnRest be true of false?
    return calcGroupFinishTime(false, false) - calcGroupFinishTime(true, false);
}

void Job::SetHosts(std::vector<const FatTree::Node *> &&hosts) {
    assert(m_Hosts.empty());
    assert(hosts.size() == HostCount);
    m_Hosts = std::move(hosts);
}

void Job::SetNextAggrTree(std::optional<FatTree::AggrTree> &&aggrTree) {
    if (m_IsRunning && m_IsUsingSharp)
        m_NextAggrTree = std::move(aggrTree);
    else
        m_AggrTree = std::move(aggrTree);
}

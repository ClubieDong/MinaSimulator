#include "job.hpp"
#include "trace.hpp"
#include <cassert>

double Job::GetNextEventTime(double now) const {
    if (!m_IsStarted)
        return now;
    assert(!m_IsFinished);
    assert(m_CurrentStepIdx < StepCount);
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
        Trace::RecordBeginJob(now, *this);
        Trace::RecordBeginStep(now, *this);
        Trace::RecordBeginGroup(now, *this);
        m_IsStarted = true;
        m_JobStartTime = now;
        m_CurrentGroupStartTime = now;
        return false;
    }
    assert(!m_IsFinished);
    assert(m_CurrentStepIdx < StepCount);
    assert(m_CurrentGroupIdx < CommOpGroups.size());
    const auto &opGroup = CommOpGroups[m_CurrentGroupIdx];
    if (m_CurrentOpIdx >= opGroup.CommOps.size()) {
        assert(!m_IsRunning);
        assert(now >= m_CurrentGroupStartTime + opGroup.SyncTime);
        Trace::RecordEndGroup(now, *this);
        m_CurrentOpIdx = 0;
        ++m_CurrentGroupIdx;
        if (m_CurrentGroupIdx >= CommOpGroups.size()) {
            Trace::RecordEndStep(now, *this);
            m_CurrentGroupIdx = 0;
            ++m_CurrentStepIdx;
            if (m_CurrentStepIdx >= StepCount) {
                Trace::RecordEndJob(now, *this);
                m_IsFinished = true;
                m_JobFinishTime = now;
                return true;
            }
            Trace::RecordBeginStep(now, *this);
        }
        Trace::RecordBeginGroup(now, *this);
        m_CurrentGroupStartTime = now;
        return false;
    }
    const auto &op = opGroup.CommOps[m_CurrentOpIdx];
    if (m_IsRunning) {
        assert(now == m_CurrentTransmissionStartTime + m_CurrentTransmissionDuration);
        Trace::RecordEndTransmission(now, *this);
        m_IsRunning = false;
        m_CurrentOpTransmittedMessageSize += m_TransmittingMessageSize;
        assert(m_CurrentOpTransmittedMessageSize <= op.MessageSize);
        if (m_CurrentOpTransmittedMessageSize == op.MessageSize) {
            Trace::RecordEndCommOp(now, *this);
            m_CurrentOpTransmittedMessageSize = 0;
            ++m_CurrentOpIdx;
        }
        (m_IsUsingSharp ? m_JobDurationWithSharp : m_JobDurationWithoutSharp) += m_CurrentTransmissionDuration;
        m_AfterTransmissionCallback(*this, now);
        return false;
    }
    if (m_CurrentOpTransmittedMessageSize == 0 && now != m_WaitingUntilTime)
        Trace::RecordBeginCommOp(now, *this);
    auto scheduleRes = m_BeforeTransmissionCallback(*this, now);
    if (scheduleRes.InsertWaitingTime) {
        Trace::RecordBeginWaiting(now, *this);
        m_WaitingUntilTime = now + scheduleRes.WaitingTime;
        return false;
    }
    if (now == m_WaitingUntilTime) {
        Trace::RecordEndWaiting(now, *this);
        m_WaitingUntilTime = 0.0;
    }
    m_IsRunning = true;
    m_IsUsingSharp = scheduleRes.UseSharp;
    m_TransmittingMessageSize = scheduleRes.MessageSize;
    assert(m_CurrentOpTransmittedMessageSize + m_TransmittingMessageSize <= op.MessageSize);
    m_CurrentTransmissionDuration =
        CalcTransmissionDuration(op.OpType, m_TransmittingMessageSize, m_IsUsingSharp, HostCount);
    m_CurrentTransmissionStartTime = now;
    Trace::RecordBeginTransmission(now, *this);
    return false;
}

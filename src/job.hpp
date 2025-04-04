#pragma once

#include "fat_tree.hpp"
#include <atomic>
#include <functional>
#include <optional>
#include <vector>

struct CommOp {
    enum class Type {
        AllReduce,
    };

    double StartTimeInGroup;
    unsigned long long MessageSize; // In Byte
    Type OpType;

    explicit CommOp(double startTimeInGroup, unsigned long long messageSize, Type opType)
        : StartTimeInGroup(startTimeInGroup), MessageSize(messageSize), OpType(opType) {}
};

struct CommOpGroup {
    std::vector<CommOp> CommOps;
    double SyncTime;
};

struct CommOpScheduleResult {
    bool InsertWaitingTime;
    double WaitingTime;
    bool UseSharp;
    unsigned long long MessageSize;

    explicit CommOpScheduleResult(double waitingTime) : InsertWaitingTime(true), WaitingTime(waitingTime) {}
    explicit CommOpScheduleResult(bool useSharp, unsigned long long messageSize = -1ull)
        : InsertWaitingTime(false), UseSharp(useSharp), MessageSize(messageSize) {}
};

struct CommOpRunningInfo {
    double GroupStartTime;
    double OpStartTime;
    double DurationWithSharp;
    double DurationWithoutSharp;
    unsigned int GroupIdx;
    unsigned int OpIdx;
};

class Job {
private:
    inline static std::atomic<unsigned int> m_NextID = 0;

    // Given the job and the current time, returns CommOpScheduleResult.
    std::function<CommOpScheduleResult(const Job &, double)> m_BeforeTransmissionCallback;
    // Given the job and the current time, returns nothing.
    std::function<void(const Job &, double)> m_AfterTransmissionCallback = [](const Job &, double) {};

    unsigned int m_CurrentStepIdx = 0;
    unsigned int m_CurrentGroupIdx = 0;
    unsigned int m_CurrentOpIdx = 0;
    unsigned long long m_CurrentOpTransmittedMessageSize = 0;
    bool m_IsRunning = false;
    double m_WaitingUntilTime = -1.0;
    unsigned long long m_TransmittingMessageSize;
    double m_CurrentTransmissionDuration;

    double m_CurrentGroupStartTime;
    double m_CurrentTransmissionStartTime;
    bool m_IsUsingSharp;

    bool m_IsStarted = false;
    bool m_IsFinished = false;
    double m_StartTime;
    double m_FinishTime;
    double m_DurationWithSharp = 0.0;
    double m_DurationWithoutSharp = 0.0;
    unsigned int m_TreeMigrationCount = 0;
    unsigned int m_ConsensusCount = 0;

    std::vector<const FatTree::Node *> m_Hosts;
    std::optional<FatTree::AggrTree> m_AggrTree;
    std::optional<std::optional<FatTree::AggrTree>> m_NextAggrTree;

    double CalcStepDuration(bool useSharp) const;

public:
    // Given CommOp type, message size, whether to use SHARP, and # of hosts, returns the duration of CommOp in seconds.
    inline static std::function<double(CommOp::Type, unsigned long long, bool, unsigned int)> CalcTransmissionDuration;

    const unsigned int ID;
    const std::string_view ModelName;
    const unsigned int HostCount;
    const std::optional<unsigned int> StepCount;
    const std::vector<CommOpGroup> CommOpGroups;

    double StepDurationWithSharp;
    double StepDurationWithoutSharp;

    explicit Job(std::string_view modelName, unsigned int hostCount, std::optional<unsigned int> stepCount);

    // Returns the time of the next event.
    double GetNextEvent(double now) const;
    // Returns whether the job is finished.
    bool RunNextEvent(double now);

    std::optional<CommOpRunningInfo> GetNextCommOpInfo(double now) const;
    double GetNextCommOpPriority(const CommOpRunningInfo &commOpInfo) const;

    unsigned int GetCurrentStepIdx() const { return m_CurrentStepIdx; }
    unsigned int GetCurrentGroupIdx() const { return m_CurrentGroupIdx; }
    unsigned int GetCurrentOpIdx() const { return m_CurrentOpIdx; }
    unsigned long long GetCurrentOpTransmittedMessageSize() const { return m_CurrentOpTransmittedMessageSize; }
    unsigned long long GetTransmittingMessageSize() const { return m_TransmittingMessageSize; }
    double GetCurrentGroupStartTime() const { return m_CurrentGroupStartTime; }
    bool IsRunning() const { return m_IsRunning; }
    bool IsUsingSharp() const { return m_IsUsingSharp; }
    bool IsStarted() const { return m_IsStarted; }
    bool IsFinished() const { return m_IsFinished; }
    double GetStartTime() const { return m_StartTime; }
    double GetFinishTime() const { return m_FinishTime; }
    double GetDurationWithSharp() const { return m_DurationWithSharp; }
    double GetDurationWithoutSharp() const { return m_DurationWithoutSharp; }
    unsigned int GetTreeMigrationCount() const { return m_TreeMigrationCount; }
    unsigned int GetConsensusCount() const { return m_ConsensusCount; }
    const std::vector<const FatTree::Node *> &GetHosts() const { return m_Hosts; }
    const std::optional<FatTree::AggrTree> &GetCurrentAggrTree() const { return m_AggrTree; }
    const std::optional<FatTree::AggrTree> &GetNextAggrTree() const {
        return m_NextAggrTree ? *m_NextAggrTree : m_AggrTree;
    }

    void SetBeforeTransmissionCallback(const decltype(m_BeforeTransmissionCallback) &callback) {
        m_BeforeTransmissionCallback = callback;
    }
    void SetAfterTransmissionCallback(const decltype(m_AfterTransmissionCallback) &callback) {
        m_AfterTransmissionCallback = callback;
    }
    void SetHosts(std::vector<const FatTree::Node *> &&hosts);
    void SetNextAggrTree(std::optional<FatTree::AggrTree> &&aggrTree);
    void IncrementConsensusCount() { ++m_ConsensusCount; }
};

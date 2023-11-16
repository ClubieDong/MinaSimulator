#pragma once

#include <functional>
#include <vector>

struct CommOp {
    enum class Type {
        AllReduce,
    };

    double StartTimeInGroup;
    unsigned long long MessageSize; // In Byte
    Type OpType;
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
    explicit CommOpScheduleResult(bool useSharp, unsigned long long messageSize)
        : InsertWaitingTime(false), UseSharp(useSharp), MessageSize(messageSize) {}
};

class Job {
private:
    inline static unsigned int m_NextID = 0;

    // Given the job and the current time, returns CommOpScheduleResult.
    std::function<CommOpScheduleResult(const Job &, double)> m_BeforeTransmissionCallback;
    // Given the job and the current time, returns nothing.
    std::function<void(const Job &, double)> m_AfterTransmissionCallback = [](const Job &, double) {};

    unsigned int m_CurrentStepIdx = 0;
    unsigned int m_CurrentGroupIdx = 0;
    unsigned int m_CurrentOpIdx = 0;
    unsigned long long m_CurrentOpTransmittedMessageSize = 0;
    bool m_IsRunning = false;
    double m_WaitingUntilTime = 0.0;
    unsigned long long m_TransmittingMessageSize;
    double m_CurrentTransmissionDuration;

    double m_CurrentGroupStartTime;
    double m_CurrentTransmissionStartTime;
    bool m_IsUsingSharp;

    bool m_IsStarted = false;
    bool m_IsFinished = false;
    double m_JobStartTime;
    double m_JobFinishTime;
    double m_JobDurationWithSharp = 0.0;
    double m_JobDurationWithoutSharp = 0.0;

public:
    // Given CommOp type, message size, whether to use SHARP, and # of hosts, returns the duration of CommOp in seconds.
    inline static std::function<double(CommOp::Type, unsigned long long, bool, unsigned int)> CalcTransmissionDuration;

    const unsigned int ID;
    const unsigned int HostCount;
    const unsigned int StepCount;
    const std::vector<CommOpGroup> CommOpGroups;

    explicit Job(unsigned int hostCount, unsigned int stepCount, std::vector<CommOpGroup> &&commOpGroups)
        : ID(m_NextID++), HostCount(hostCount), StepCount(stepCount), CommOpGroups(std::move(commOpGroups)) {}

    // Returns the time of the next event.
    double GetNextEvent(double now) const;
    // Returns whether the job is finished.
    bool RunNextEvent(double now);

    unsigned int GetCurrentStepIdx() const { return m_CurrentStepIdx; }
    unsigned int GetCurrentGroupIdx() const { return m_CurrentGroupIdx; }
    unsigned int GetCurrentOpIdx() const { return m_CurrentOpIdx; }
    unsigned long long GetCurrentOpTransmittedMessageSize() const { return m_CurrentOpTransmittedMessageSize; }
    unsigned long long GetTransmittingMessageSize() const { return m_TransmittingMessageSize; }
    bool IsStarted() const { return m_IsStarted; }
    bool IsFinished() const { return m_IsFinished; }
    double GetJobStartTime() const { return m_JobStartTime; }
    double GetJobFinishTime() const { return m_JobFinishTime; }
    double GetJobDurationWithSharp() const { return m_JobDurationWithSharp; }
    double GetJobDurationWithoutSharp() const { return m_JobDurationWithoutSharp; }
    const CommOp &GetCurrentCommOp() const { return CommOpGroups[m_CurrentGroupIdx].CommOps[m_CurrentOpIdx]; }

    void SetBeforeTransmissionCallback(const decltype(m_BeforeTransmissionCallback) &callback) {
        m_BeforeTransmissionCallback = callback;
    }
    void SetAfterTransmissionCallback(const decltype(m_AfterTransmissionCallback) &callback) {
        m_AfterTransmissionCallback = callback;
    }
};

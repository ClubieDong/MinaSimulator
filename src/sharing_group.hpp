#pragma once

#include "fat_tree_resource.hpp"
#include "job.hpp"
#include <functional>
#include <vector>

class SharingGroup {
private:
    FatTreeResource *m_Resources;

    // Given the job, the current time, and whether to use SHARP, returns nothing.
    std::function<void(const Job &, double, bool)> m_BeforeTransmissionCallback = [](const Job &, double, bool) {};
    // Given the job, the current time, and whether to use SHARP, returns nothing.
    std::function<void(const Job &, double, bool)> m_AfterTransmissionCallback = [](const Job &, double, bool) {};
    // Given the sharing group, the job, and the current time, returns CommOpScheduleResult.
    std::function<CommOpScheduleResult(const SharingGroup &, const Job &, double)> m_SharingPolicy;

public:
    inline static bool RecordSharingOverhead = false;
    inline static unsigned int SharingPolicyCallCount = 0;
    inline static unsigned long long SharingPolicyOverhead = 0; // In nanosecond

    const std::vector<Job *> Jobs;

    explicit SharingGroup(std::vector<Job *> &&jobs, FatTreeResource *resources,
                          const decltype(m_SharingPolicy) &sharingPolicy);

    // Returns the time of the next event and the job that will run next.
    std::pair<double, Job *> GetNextEvent(double now) const;
    // Returns whether the job is finished.
    bool RunNextEvent(double now, Job *job) { return job->RunNextEvent(now); }

    bool CanUseSharp(const Job &job) const;

    FatTreeResource *GetFatTreeResources() const { return m_Resources; }

    void SetBeforeTransmissionCallback(const decltype(m_BeforeTransmissionCallback) &callback) {
        m_BeforeTransmissionCallback = callback;
    }
    void SetAfterTransmissionCallback(const decltype(m_AfterTransmissionCallback) &callback) {
        m_AfterTransmissionCallback = callback;
    }
};

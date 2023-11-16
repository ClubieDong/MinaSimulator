#pragma once

#include "job.hpp"
#include <functional>
#include <vector>

class SharingGroup {
private:
    // Given the job, the current time, and whether to use SHARP, returns nothing.
    std::function<void(const Job &, double, bool)> m_BeforeTransmissionCallback = [](const Job &, double, bool) {};
    // Given the job and the current time, returns nothing.
    std::function<void(const Job &, double)> m_AfterTransmissionCallback = [](const Job &, double) {};
    // Given the sharing group, the job, and the current time, returns CommOpScheduleResult.

public:
    const std::vector<Job *> Jobs;
    std::function<CommOpScheduleResult(const SharingGroup &, const Job &, double)> SharingPolicy;

    explicit SharingGroup(std::vector<Job *> &&jobs, decltype(SharingPolicy) &&sharingPolicy);

    // Returns the time of the next event and the job that will run next.
    std::pair<double, Job *> GetNextEvent(double now) const;
    // Returns whether the job is finished.
    bool RunNextEvent(double now, Job *job) { return job->RunNextEvent(now); }

    void SetBeforeTransmissionCallback(const decltype(m_BeforeTransmissionCallback) &callback) {
        m_BeforeTransmissionCallback = callback;
    }
    void SetAfterTransmissionCallback(const decltype(m_AfterTransmissionCallback) &callback) {
        m_AfterTransmissionCallback = callback;
    }
};

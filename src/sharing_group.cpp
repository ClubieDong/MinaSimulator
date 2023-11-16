#include "sharing_group.hpp"
#include <cassert>

SharingGroup::SharingGroup(std::vector<Job *> &&jobs, const decltype(m_SharingPolicy) &sharingPolicy)
    : m_SharingPolicy(sharingPolicy), Jobs(std::move(jobs)) {
    for (auto job : Jobs) {
        // TODO: Migrating
        job->SetBeforeTransmissionCallback([this](const Job &job, double now) -> CommOpScheduleResult {
            auto res = m_SharingPolicy(*this, job, now);
            if (!res.InsertWaitingTime)
                m_BeforeTransmissionCallback(job, now, res.UseSharp);
            return res;
        });
        job->SetAfterTransmissionCallback(
            [this](const Job &job, double now) { m_AfterTransmissionCallback(job, now, job.IsUsingSharp()); });
    }
}

std::pair<double, Job *> SharingGroup::GetNextEvent(double now) const {
    double nearestEventTime;
    Job *nearestJob = nullptr;
    for (auto job : Jobs) {
        auto nextEventTime = job->GetNextEvent(now);
        if (!nearestJob || nextEventTime < nearestEventTime) {
            nearestEventTime = nextEventTime;
            nearestJob = job;
        }
    }
    assert(nearestJob);
    return {nearestEventTime, nearestJob};
}

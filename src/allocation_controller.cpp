#include "allocation_controller.hpp"
#include "utils/union_find.hpp"
#include <cassert>

void AllocationController::BuildSharingGroups() {
    UnionFind u(m_RunningJobs.size());
    for (unsigned int i = 0; i < m_RunningJobs.size(); ++i) {
        const auto &aggrTree1 = m_RunningJobs[i]->GetNextAggrTree();
        if (!aggrTree1)
            continue;
        for (unsigned int j = i + 1; j < m_RunningJobs.size(); ++j) {
            const auto &aggrTree2 = m_RunningJobs[j]->GetNextAggrTree();
            if (!aggrTree2)
                continue;
            if (FatTreeResource::CheckTreeConflict(*aggrTree1, *aggrTree2))
                u.Union(i, j);
        }
    }
    m_SharingGroups.clear();
    for (const auto &[_, group] : u.Group()) {
        std::vector<Job *> jobs;
        for (auto i : group)
            jobs.push_back(m_RunningJobs[i].get());
        m_SharingGroups.emplace_back(std::move(jobs), m_SharingPolicy);
        m_SharingGroups.back().SetBeforeTransmissionCallback([this](const Job &job, double, bool useSharp) {
            if (useSharp) {
                const auto &aggrTree = job.GetCurrentAggrTree();
                assert(aggrTree);
                m_Resources.Allocate(*aggrTree);
            }
        });
        m_SharingGroups.back().SetAfterTransmissionCallback([this](const Job &job, double, bool useSharp) {
            if (useSharp) {
                const auto &aggrTree = job.GetCurrentAggrTree();
                assert(aggrTree);
                m_Resources.Allocate(*aggrTree);
            }
        });
    }
}

void AllocationController::RunNewJobs(bool rebuildSharingGroups) {
    std::vector<Job *> newJobs;
    while (m_NextJob) {
        auto hosts = m_HostAllocationPolicy(m_Resources, m_NextJob->HostCount);
        if (!hosts)
            break;
        m_Resources.Allocate(*hosts);
        m_NextJob->SetHosts(std::move(*hosts));
        newJobs.push_back(m_NextJob.get());
        m_RunningJobs.push_back(std::move(m_NextJob));
        m_NextJob = m_GetNextJob();
    }
    if (!newJobs.empty())
        m_TreeBuildingPolicy(m_Resources, m_RunningJobs, newJobs);
    if (!newJobs.empty() || rebuildSharingGroups)
        BuildSharingGroups();
}

std::tuple<double, Job *, SharingGroup *> AllocationController::GetNextEvent(double now) {
    double nearestEventTime;
    Job *nearestJob = nullptr;
    SharingGroup *nearestSharingGroup = nullptr;
    for (auto &sharingGroup : m_SharingGroups) {
        auto [nextEventTime, nextJob] = sharingGroup.GetNextEvent(now);
        if (!nearestJob || nextEventTime < nearestEventTime) {
            nearestEventTime = nextEventTime;
            nearestJob = nextJob;
            nearestSharingGroup = &sharingGroup;
        }
    }
    assert(nearestJob);
    return {nearestEventTime, nearestJob, nearestSharingGroup};
}

AllocationController::AllocationController(FatTreeResource &&resources, decltype(m_GetNextJob) &&getNextJob,
                                           decltype(m_HostAllocationPolicy) &&hostAllocationPolicy,
                                           decltype(m_TreeBuildingPolicy) &&treeBuildingPolicy,
                                           decltype(m_SharingPolicy) &&sharingPolicy)
    : m_GetNextJob(std::move(getNextJob)), m_HostAllocationPolicy(std::move(hostAllocationPolicy)),
      m_TreeBuildingPolicy(std::move(treeBuildingPolicy)), m_SharingPolicy(std::move(sharingPolicy)),
      m_Resources(std::move(resources)), m_NextJob(m_GetNextJob()) {}

void AllocationController::RunSimulation() {
    RunNewJobs(false);
    double now = 0.0;
    while (!m_RunningJobs.empty()) {
        auto [nextTime, job, sharingGroup] = GetNextEvent(now);
        assert(nextTime >= now);
        now = nextTime;
        auto jobFinished = sharingGroup->RunNextEvent(now, job);
        if (jobFinished) {
            m_Resources.Deallocate(job->GetHosts());
            for (auto iter = m_RunningJobs.cbegin(); iter != m_RunningJobs.cend(); ++iter)
                if (iter->get() == job) {
                    m_RunningJobs.erase(iter);
                    break;
                }
            RunNewJobs(true);
        }
    }
}

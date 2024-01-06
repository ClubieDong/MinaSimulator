#include "allocation_controller.hpp"
#include "utils/union_find.hpp"
#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>

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
            if (m_Resources.CheckTreeConflict(*aggrTree1, *aggrTree2))
                u.Union(i, j);
        }
    }
    m_SharingGroups.clear();
    for (const auto &[_, group] : u.Group()) {
        std::vector<Job *> jobs;
        for (auto i : group)
            jobs.push_back(m_RunningJobs[i].get());
        auto sharingGroup = std::make_unique<SharingGroup>(std::move(jobs), &m_Resources, m_SharingPolicy);
        sharingGroup->SetBeforeTransmissionCallback([this](const Job &job, double, bool useSharp) {
            if (useSharp) {
                const auto &aggrTree = job.GetCurrentAggrTree();
                assert(aggrTree);
                m_Resources.Allocate(*aggrTree);
            }
        });
        sharingGroup->SetAfterTransmissionCallback([this](const Job &job, double, bool useSharp) {
            if (useSharp) {
                const auto &aggrTree = job.GetCurrentAggrTree();
                assert(aggrTree);
                m_Resources.Deallocate(*aggrTree);
            }
        });
        m_SharingGroups.push_back(std::move(sharingGroup));
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
        ++m_AllocatedJobCount;
        m_NextJob = m_GetNextJob();
    }
    if (!newJobs.empty())
        m_TreeBuildingPolicy(m_Resources, m_RunningJobs, newJobs);
    if (RecordTreeConflicts) {
        for (auto job : newJobs)
            m_TreeConflictTrace.push_back(!job->GetNextAggrTree().has_value());
        m_HostFragmentTrace[m_AllocatedJobCount] = {
            m_Resources.CalcHostFragments(false),
            m_Resources.CalcHostFragments(true),
        };
    }
    if (!newJobs.empty() || rebuildSharingGroups)
        BuildSharingGroups();
}

std::tuple<double, Job *, SharingGroup *> AllocationController::GetNextEvent(double now) {
    double nearestEventTime;
    Job *nearestJob = nullptr;
    SharingGroup *nearestSharingGroup = nullptr;
    for (auto &sharingGroup : m_SharingGroups) {
        auto [nextEventTime, nextJob] = sharingGroup->GetNextEvent(now);
        if (!nearestJob || nextEventTime < nearestEventTime) {
            nearestEventTime = nextEventTime;
            nearestJob = nextJob;
            nearestSharingGroup = sharingGroup.get();
        }
    }
    assert(nearestJob);
    return {nearestEventTime, nearestJob, nearestSharingGroup};
}

void AllocationController::ShowProgress(double now, bool last) {
    auto realNow = std::chrono::high_resolution_clock::now();
    if (m_LastShowProgressTime) {
        if (!last && (realNow - *m_LastShowProgressTime) < std::chrono::milliseconds(20))
            return;
        std::cout << '\r';
    }
    m_LastShowProgressTime = realNow;
    std::cout << std::setprecision(6) << std::fixed;
    std::cout << "Simulation progress: " << now << 's';
    if (m_MaxSimulationTime) {
        std::cout << " / " << *m_MaxSimulationTime << "s (";
        std::cout << std::setprecision(1) << std::fixed;
        std::cout << (now / *m_MaxSimulationTime * 100) << "%)";
    }
    if (last)
        std::cout << '\n';
    std::cout << std::flush;
}

AllocationController::AllocationController(FatTreeResource &&resources, decltype(m_GetNextJob) &&getNextJob,
                                           HostAllocationPolicy &&hostAllocationPolicy,
                                           TreeBuildingPolicy &&treeBuildingPolicy, SharingPolicy &&sharingPolicy)
    : m_GetNextJob(std::move(getNextJob)), m_HostAllocationPolicy(std::move(hostAllocationPolicy)),
      m_TreeBuildingPolicy(std::move(treeBuildingPolicy)), m_SharingPolicy(std::move(sharingPolicy)),
      m_Resources(std::move(resources)), m_NextJob(m_GetNextJob()) {}

AllocationController::~AllocationController() {
    if (!RecordTreeConflicts)
        return;
    nlohmann::json data = {
        {"host_fragments", m_HostFragmentTrace},
        {"tree_conflicts", m_TreeConflictTrace},
    };
    std::ofstream file("tree_conflict_trace.json");
    file << data.dump();
}

SimulationResult AllocationController::RunSimulation(std::optional<double> maxSimulationTime, bool showProgress) {
    m_MaxSimulationTime = maxSimulationTime;
    m_LastShowProgressTime = std::nullopt;
    SimulationResult result;
    double now = 0.0;
    RunNewJobs(false);
    if (showProgress)
        ShowProgress(now, false);
    while (!m_RunningJobs.empty() && (!m_MaxSimulationTime || now <= *m_MaxSimulationTime)) {
        auto [nextTime, job, sharingGroup] = GetNextEvent(now);
        assert(nextTime >= now);
        now = nextTime;
        if (showProgress)
            ShowProgress(now, false);
        auto jobFinished = sharingGroup->RunNextEvent(now, job);
        if (jobFinished) {
            assert(job->StepCount);
            result.TotalHostTime += (job->GetFinishTime() - job->GetStartTime()) * job->HostCount;
            result.TotalJCT += job->GetFinishTime() - job->GetStartTime();
            result.TotalJCTWithSharp += job->StepDurationWithSharp * *job->StepCount;
            result.TotalJCTWithoutSharp += job->StepDurationWithoutSharp * *job->StepCount;
            m_Resources.Deallocate(job->GetHosts());
            for (auto iter = m_RunningJobs.cbegin(); iter != m_RunningJobs.cend(); ++iter)
                if (iter->get() == job) {
                    m_RunningJobs.erase(iter);
                    break;
                }
            RunNewJobs(true);
        }
    }
    if (showProgress)
        ShowProgress(now, true);
    for (const auto &job : m_RunningJobs) {
        result.TotalHostTime += (now - job->GetStartTime()) * job->HostCount;
        result.TotalJCT += job->GetCurrentGroupStartTime() - job->GetStartTime();
        result.TotalJCTWithSharp += job->StepDurationWithSharp * job->GetCurrentStepIdx();
        result.TotalJCTWithoutSharp += job->StepDurationWithoutSharp * job->GetCurrentStepIdx();
    }
    result.SimulatedTime = now;
    result.ClusterUtilization = result.TotalHostTime / (now * m_Resources.Topology->NodesByLayer[0].size());
    result.JCTScore =
        (result.TotalJCT - result.TotalJCTWithoutSharp) / (result.TotalJCTWithSharp - result.TotalJCTWithoutSharp);
    return result;
}

#include "allocation_controller.hpp"
#include "host_allocation_policies/first.hpp"
#include "tree_building_policies/first.hpp"
#include "utils/union_find.hpp"
#include <cassert>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>

void to_json(nlohmann::json &json, const SimulationResult &result) {
    json = {
        {"JCTScore", result.JCTScore()},
        {"JCTScoreWeighted", result.JCTScoreWeighted()},
        {"SharpRatio", result.SharpRatio()},
        {"SharpRatioWeighted", result.SharpRatioWeighted()},
        {"FinishedJobCount", result.FinishedJobCount},
        {"SimulatedTime", result.SimulatedTime},
        {"EventCount", result.EventCount},
        {"ClusterUtilization", result.ClusterUtilization},
        {"SharpUtilization", result.SharpUtilization},
        {"TotalJCT", result.TotalJCT},
        {"TotalJCTWeighted", result.TotalJCTWeighted},
        {"TotalJCTWithSharp", result.TotalJCTWithSharp},
        {"TotalJCTWithSharpWeighted", result.TotalJCTWithSharpWeighted},
        {"TotalJCTWithoutSharp", result.TotalJCTWithoutSharp},
        {"TotalJCTWithoutSharpWeighted", result.TotalJCTWithoutSharpWeighted},
        {"TotalSharpTime", result.TotalSharpTime},
        {"TotalSharpTimeWeighted", result.TotalSharpTimeWeighted},
        {"TotalSharpUsage", result.TotalSharpUsage},
        {"TimeCostHostAllocation", result.TimeCostHostAllocation},
        {"TimeCostTreeBuilding", result.TimeCostTreeBuilding},
        {"TreeMigrationCount", result.TreeMigrationCount},
        {"SharpEnabledJobCount", result.SharpEnabledJobCount},
        {"ConsensusFrequency", result.ConsensusFrequency},
    };
}

std::ostream &operator<<(std::ostream &os, const SimulationResult &result) {
    os << std::setprecision(6) << std::fixed;
    os << "  JCTScore:                     " << result.JCTScore() << '\n';
    os << "  JCTScoreWeighted:             " << result.JCTScoreWeighted() << '\n';
    os << "  SharpRatio:                   " << result.SharpRatio() * 100 << "%\n";
    os << "  SharpRatioWeighted:           " << result.SharpRatioWeighted() * 100 << "%\n";
    os << "  FinishedJobCount:             " << result.FinishedJobCount << '\n';
    os << "  SimulatedTime:                " << result.SimulatedTime << " sec\n";
    os << "  EventCount:                   " << result.EventCount << '\n';
    os << "  ClusterUtilization:           " << result.ClusterUtilization * 100 << "%\n";
    os << "  SharpUtilization:             " << result.SharpUtilization * 100 << "%\n";
    os << "  TotalJCT:                     " << result.TotalJCT << " sec\n";
    os << "  TotalJCTWeighted:             " << result.TotalJCTWeighted << " sec\n";
    os << "  TotalJCTWithSharp:            " << result.TotalJCTWithSharp << " sec\n";
    os << "  TotalJCTWithSharpWeighted:    " << result.TotalJCTWithSharpWeighted << " sec\n";
    os << "  TotalJCTWithoutSharp:         " << result.TotalJCTWithoutSharp << " sec\n";
    os << "  TotalJCTWithoutSharpWeighted: " << result.TotalJCTWithoutSharpWeighted << " sec\n";
    os << "  TotalSharpTime:               " << result.TotalSharpTime << " sec\n";
    os << "  TotalSharpTimeWeighted:       " << result.TotalSharpTimeWeighted << " sec\n";
    os << "  TotalSharpUsage:              " << result.TotalSharpUsage << " sec\n";
    os << "  TimeCostHostAllocation:       " << result.TimeCostHostAllocation << " ms (wall clock)\n";
    os << "  TimeCostTreeBuilding:         " << result.TimeCostTreeBuilding << " ms (wall clock)\n";
    os << "  TreeMigrationCount:           " << result.TreeMigrationCount << '\n';
    os << "  SharpEnabledJobCount:         " << result.SharpEnabledJobCount << '\n';
    os << "  ConsensusFrequency:           " << result.ConsensusFrequency << " times per sec\n";
    return os;
}

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

void AllocationController::RunNewJobs(SimulationResult &result) {
    std::vector<Job *> newJobs;
    while (m_NextJob) {
        auto start = std::chrono::high_resolution_clock::now();
        auto hosts = m_HostAllocationPolicy(m_Resources, m_NextJob->HostCount);
        auto finish = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start);
        result.TimeCostHostAllocation += duration.count() / 1000.0;
        if (!hosts)
            break;
        m_Resources.Allocate(*hosts);
        m_NextJob->SetHosts(std::move(*hosts));
        newJobs.push_back(m_NextJob.get());
        m_RunningJobs.push_back(std::move(m_NextJob));
        ++m_AllocatedJobCount;
        if (RecordClusterState.has_value() && RecordClusterState.value() == m_AllocatedJobCount) {
            std::vector<std::vector<unsigned int>> clusterState;
            for (const auto &job : m_RunningJobs) {
                clusterState.emplace_back();
                for (const auto &node : job->GetHosts())
                    clusterState.back().push_back(node->ID);
            }
            std::ofstream file(ClusterStateOutputFile);
            file << nlohmann::json(clusterState);
        }
        m_NextJob = m_GetNextJob();
    }
    auto start = std::chrono::high_resolution_clock::now();
    m_TreeBuildingPolicy(m_Resources, m_RunningJobs, newJobs);
    auto finish = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start);
    result.TimeCostTreeBuilding += duration.count() / 1000.0;
    if (RecordTreeConflicts) {
        for (auto job : newJobs)
            m_TreeConflictTrace.push_back(!job->GetNextAggrTree().has_value());
        m_HostFragmentTrace[m_AllocatedJobCount] = {
            m_Resources.CalcHostFragments(false),
            m_Resources.CalcHostFragments(true),
        };
    }
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
    std::ofstream file("results/tree_conflict_trace.json");
    file << data;
}

SimulationResult AllocationController::RunSimulation(std::optional<double> maxSimulationTime, bool showProgress) {
    m_MaxSimulationTime = maxSimulationTime;
    m_LastShowProgressTime = std::nullopt;
    SimulationResult result;
    double now = 0.0;
    RunNewJobs(result);
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
            ++result.FinishedJobCount;
            result.TotalJCT += job->GetFinishTime() - job->GetStartTime();
            result.TotalJCTWeighted += (job->GetFinishTime() - job->GetStartTime()) * job->HostCount;
            result.TotalJCTWithSharp += job->StepDurationWithSharp * *job->StepCount;
            result.TotalJCTWithSharpWeighted += job->StepDurationWithSharp * *job->StepCount * job->HostCount;
            result.TotalJCTWithoutSharp += job->StepDurationWithoutSharp * *job->StepCount;
            result.TotalJCTWithoutSharpWeighted += job->StepDurationWithoutSharp * *job->StepCount * job->HostCount;
            result.TotalSharpTime += job->GetDurationWithSharp();
            result.TotalSharpTimeWeighted += job->GetDurationWithSharp() * job->HostCount;
            result.TreeMigrationCount += job->GetTreeMigrationCount();
            result.ConsensusFrequency += job->GetConsensusCount();
            if (m_Resources.NodeQuota) {
                const auto &aggrTree = job->GetCurrentAggrTree();
                if (aggrTree) {
                    auto occupiedSharpResource = aggrTree->Nodes.size() - job->HostCount;
                    result.TotalSharpUsage += job->GetDurationWithSharp() * occupiedSharpResource;
                }
            }
            m_Resources.Deallocate(job->GetHosts());
            for (auto iter = m_RunningJobs.cbegin(); iter != m_RunningJobs.cend(); ++iter)
                if (iter->get() == job) {
                    m_RunningJobs.erase(iter);
                    break;
                }
            RunNewJobs(result);
        }
        ++result.EventCount;
    }
    if (showProgress)
        ShowProgress(now, true);
    for (const auto &job : m_RunningJobs) {
        result.TotalJCT += job->GetCurrentGroupStartTime() - job->GetStartTime();
        result.TotalJCTWeighted += (job->GetCurrentGroupStartTime() - job->GetStartTime()) * job->HostCount;
        result.TotalJCTWithSharp += job->StepDurationWithSharp * job->GetCurrentStepIdx();
        result.TotalJCTWithSharpWeighted += job->StepDurationWithSharp * job->GetCurrentStepIdx() * job->HostCount;
        result.TotalJCTWithoutSharp += job->StepDurationWithoutSharp * job->GetCurrentStepIdx();
        result.TotalJCTWithoutSharpWeighted +=
            job->StepDurationWithoutSharp * job->GetCurrentStepIdx() * job->HostCount;
        result.TotalSharpTime += job->GetDurationWithSharp();
        result.TotalSharpTimeWeighted += job->GetDurationWithSharp() * job->HostCount;
        result.TreeMigrationCount += job->GetTreeMigrationCount();
        if (m_Resources.NodeQuota) {
            const auto &aggrTree = job->GetCurrentAggrTree();
            if (aggrTree) {
                auto occupiedSharpResource = aggrTree->Nodes.size() - job->HostCount;
                result.TotalSharpUsage += job->GetDurationWithSharp() * occupiedSharpResource;
            }
        }
    }
    result.SimulatedTime = now;
    result.ClusterUtilization = result.TotalJCTWeighted / (now * m_Resources.Topology->NodesByLayer[0].size());
    result.ConsensusFrequency /= result.TotalJCT;
    if (m_Resources.NodeQuota) {
        const auto &topology = *m_Resources.Topology;
        auto switchCount = topology.Nodes.size() - topology.NodesByLayer[0].size();
        result.SharpUtilization = result.TotalSharpUsage / (now * switchCount);
    }
    return result;
}

SimulationResult
AllocationController::SimulateSharingGroup(const std::vector<std::pair<unsigned int, std::string_view>> &jobInfos,
                                           SharingPolicy &&sharingPolicy, unsigned int stepCount) {
    // Count the total number of hosts
    unsigned int totalHostCount = 0;
    for (const auto &[hostCount, _] : jobInfos)
        totalHostCount += hostCount;
    // Build jobs and calculate the max step duration
    std::vector<std::unique_ptr<Job>> jobs;
    double maxStepDuration = 0.0;
    jobs.reserve(jobInfos.size());
    for (const auto &[hostCount, modelName] : jobInfos) {
        auto job = std::make_unique<Job>(modelName, hostCount, std::nullopt);
        maxStepDuration = std::max(maxStepDuration, job->StepDurationWithoutSharp);
        jobs.push_back(std::move(job));
    }
    auto getNextJob = [&jobs, jobCount = 0u]() mutable -> std::unique_ptr<Job> {
        if (jobCount >= jobs.size())
            return nullptr;
        ++jobCount;
        return std::move(jobs[jobCount - 1]);
    };
    // Build allocation controller
    FatTree topology({totalHostCount, 1, 1}, {1, 1, 1});
    FatTreeResource resources(topology, 1, std::nullopt);
    FirstHostAllocationPolicy hostAllocationPolicy;
    FirstTreeBuildingPolicy treeBuildingPolicy(false);
    AllocationController controller(std::move(resources), std::move(getNextJob), std::move(hostAllocationPolicy),
                                    std::move(treeBuildingPolicy), std::move(sharingPolicy));
    // Simulate
    return controller.RunSimulation(stepCount * maxStepDuration, false);
}

#pragma once

#include "fat_tree_resource.hpp"
#include "job.hpp"
#include "sharing_group.hpp"
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

struct SimulationResult {
    double SimulatedTime;
    double ClusterUtilization;
    double JCTScore;

    double TotalHostTime = 0.0;
    double TotalJCT = 0.0;
    double TotalJCTWithSharp = 0.0;
    double TotalJCTWithoutSharp = 0.0;
};

class AllocationController {
public:
    using HostAllocationPolicy =
        std::function<std::optional<std::vector<const FatTree::Node *>>(const FatTreeResource &, unsigned int)>;
    using TreeBuildingPolicy = std::function<void(const FatTreeResource &, const std::vector<std::unique_ptr<Job>> &,
                                                  const std::vector<Job *> &)>;
    using SharingPolicy = std::function<CommOpScheduleResult(const SharingGroup &, const Job &, double)>;

private:
    // Returns the next job if exists, nullptr if not.
    std::function<std::unique_ptr<Job>()> m_GetNextJob;
    // Given the resources and the number of required hosts, returns a vector of hosts if there are enough available
    // hosts, std::nullopt if not.
    HostAllocationPolicy m_HostAllocationPolicy;
    // Given the resources, all the running jobs, and the new jobs, build the aggregation tree of each job. This
    // function should set the aggregation trees by calling Job::SetNextAggrTree.
    TreeBuildingPolicy m_TreeBuildingPolicy;
    // Given the sharing group, the job, and the current time, returns CommOpScheduleResult.
    SharingPolicy m_SharingPolicy;

    FatTreeResource m_Resources;
    std::vector<std::unique_ptr<Job>> m_RunningJobs;
    std::vector<std::unique_ptr<SharingGroup>> m_SharingGroups;
    std::unique_ptr<Job> m_NextJob;

    std::optional<double> m_MaxSimulationTime; // In second
    std::optional<std::chrono::high_resolution_clock::time_point> m_LastShowProgressTime;

    void BuildSharingGroups();
    void RunNewJobs(bool rebuildSharingGroups);
    // Returns the time of the next event, the job that will run next, and the sharing group of that job.
    std::tuple<double, Job *, SharingGroup *> GetNextEvent(double now);
    void ShowProgress(double now, bool last);

public:
    explicit AllocationController(FatTreeResource &&resources, decltype(m_GetNextJob) &&getNextJob,
                                  HostAllocationPolicy &&hostAllocationPolicy, TreeBuildingPolicy &&treeBuildingPolicy,
                                  SharingPolicy &&sharingPolicy);

    SimulationResult RunSimulation(std::optional<double> maxSimulationTime, bool showProgress);
};

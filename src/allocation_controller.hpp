#pragma once

#include "fat_tree_resource.hpp"
#include "job.hpp"
#include "sharing_group.hpp"
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <unordered_map>
#include <vector>

struct SimulationResult {
    // The number of finished jobs
    unsigned int FinishedJobCount = 0;
    // Total simulation time, in second
    double SimulatedTime = 0.0;
    // Simulated event count;
    unsigned int EventCount = 0;
    // The utilization of all hosts (time in use / total time)
    double ClusterUtilization = 0.0;
    // The utilization of all SHARP switches (time in use / total time)
    double SharpUtilization = 0.0;

    // The sum of JCT of all jobs, in second
    double TotalJCT = 0.0;
    // The sum of weighted JCT of all jobs, in second
    double TotalJCTWeighted = 0.0;
    // The expected sum of JCT of all jobs with all AllReduce using SHARP, in second
    double TotalJCTWithSharp = 0.0;
    // The expected sum of weighted JCT of all jobs with all AllReduce using SHARP, in second
    double TotalJCTWithSharpWeighted = 0.0;
    // The expected sum of JCT of all jobs with all AllReduce not using SHARP, in second
    double TotalJCTWithoutSharp = 0.0;
    // The expected sum of weighted JCT of all jobs with all AllReduce not using SHARP, in second
    double TotalJCTWithoutSharpWeighted = 0.0;
    // The sum of the time of all jobs that SHARP is enabled, in second
    double TotalSharpTime = 0.0;
    // The weighted sum of the time of all jobs that SHARP is enabled, in second
    double TotalSharpTimeWeighted = 0.0;
    // The sum of the time of all switches that SHARP is enabled, in second
    double TotalSharpUsage = 0.0;

    // The profiled time spent on host allocation, in millisecond (wall clock)
    double TimeCostHostAllocation = 0.0;
    // The profiled time spent on tree building, in millisecond (wall clock)
    double TimeCostTreeBuilding = 0.0;
    // The total times of tree migration
    unsigned int TreeMigrationCount = 0;
    // The number of jobs with SHARP enabled (assigned with an aggregation tree)
    unsigned int SharpEnabledJobCount = 0;
    // The frequency of consensus protocol, in Hz
    double ConsensusFrequency = 0.0;

    // INA efficiency score, see paper for details
    double JCTScore() const { return (TotalJCT - TotalJCTWithoutSharp) / (TotalJCTWithSharp - TotalJCTWithoutSharp); }

    // Weighted INA efficiency score, see paper for details
    double JCTScoreWeighted() const {
        return (TotalJCTWeighted - TotalJCTWithoutSharpWeighted) /
               (TotalJCTWithSharpWeighted - TotalJCTWithoutSharpWeighted);
    }

    // INA utilization rate, see paper for details
    double SharpRatio() const { return TotalSharpTime / TotalJCT; }

    // Weighted INA utilization rate, see paper for details
    double SharpRatioWeighted() const { return TotalSharpTimeWeighted / TotalJCTWeighted; }

    friend void to_json(nlohmann::json &json, const SimulationResult &result);
    friend std::ostream &operator<<(std::ostream &os, const SimulationResult &result);
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
    unsigned int m_AllocatedJobCount = 0;

    std::optional<double> m_MaxSimulationTime; // In second
    std::optional<std::chrono::high_resolution_clock::time_point> m_LastShowProgressTime;

    std::unordered_map<unsigned int, std::pair<unsigned int, unsigned int>> m_HostFragmentTrace;
    std::vector<bool> m_TreeConflictTrace;

    void BuildSharingGroups();
    void RunNewJobs(SimulationResult &result);
    // Returns the time of the next event, the job that will run next, and the sharing group of that job.
    std::tuple<double, Job *, SharingGroup *> GetNextEvent(double now);
    void ShowProgress(double now, bool last);

public:
    bool RecordTreeConflicts = false;
    std::optional<unsigned int> RecordClusterState = std::nullopt;
    std::string ClusterStateOutputFile;

    explicit AllocationController(FatTreeResource &&resources, decltype(m_GetNextJob) &&getNextJob,
                                  HostAllocationPolicy &&hostAllocationPolicy, TreeBuildingPolicy &&treeBuildingPolicy,
                                  SharingPolicy &&sharingPolicy);
    ~AllocationController();

    SimulationResult RunSimulation(std::optional<double> maxSimulationTime, bool showProgress);

    static SimulationResult SimulateSharingGroup(const std::vector<std::pair<unsigned int, std::string_view>> &jobInfos,
                                                 SharingPolicy &&sharingPolicy, unsigned int stepCount);
};

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
    // Total simulation time, in second (simulation)
    double SimulatedTime;
    // The utilization of all hosts (time in use / total time)
    double ClusterUtilization;
    // INA efficiency score, see paper for details
    double JCTScore;
    // INA utilization rate, see paper for details, equals TotalSharpTime / TotalJCT
    double SharpRatio;
    // The utilization of all SHARP switches (time in use / total time)
    double SharpUtilization = 0.0;

    // The total time of all hosts running jobs, in second (simulation)
    double TotalHostTime = 0.0;
    // The sum of JCT of all jobs, in second (simulation)
    double TotalJCT = 0.0;
    // The sum of JCT of all jobs with SHARP enabled, in second (simulation)
    double TotalJCTWithSharp = 0.0;
    // The sum of JCT of all jobs with SHARP disabled, in second (simulation)
    double TotalJCTWithoutSharp = 0.0;
    // The expected sum of JCT with all AllReduce's of all jobs using SHARP, in second (simulation)
    double TotalSharpTime = 0.0;
    // The expected sum of JCT with all AllReduce's of all jobs not using SHARP, in second (simulation)
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

    explicit AllocationController(FatTreeResource &&resources, decltype(m_GetNextJob) &&getNextJob,
                                  HostAllocationPolicy &&hostAllocationPolicy, TreeBuildingPolicy &&treeBuildingPolicy,
                                  SharingPolicy &&sharingPolicy);
    ~AllocationController();

    SimulationResult RunSimulation(std::optional<double> maxSimulationTime, bool showProgress);

    static SimulationResult SharingGroupSimulation(const std::vector<const char *> &modelList,
                                                   SharingPolicy &&sharingPolicy, double gpuSpeedupRatio,
                                                   double simulationTime);
};

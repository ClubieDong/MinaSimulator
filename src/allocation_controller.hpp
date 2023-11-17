#pragma once

#include "fat_tree_resource.hpp"
#include "job.hpp"
#include "sharing_group.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <vector>

class AllocationController {
private:
    // Returns the next job if exists, nullptr if not.
    std::function<std::unique_ptr<Job>()> m_GetNextJob;
    // Given the resources and the number of required hosts, returns a vector of hosts if there are enough available
    // hosts, std::nullopt if not.
    std::function<std::optional<std::vector<const FatTree::Node *>>(const FatTreeResource &, unsigned int)>
        m_HostAllocationPolicy;
    // Given the resources, all the running jobs, and the new jobs, build the aggregation tree of each job. This
    // function should set the aggregation trees by calling Job::SetNextAggrTree.
    std::function<void(const FatTreeResource &, const std::vector<std::unique_ptr<Job>> &, const std::vector<Job *> &)>
        m_TreeBuildingPolicy;
    // Given the sharing group, the job, and the current time, returns CommOpScheduleResult.
    std::function<CommOpScheduleResult(const SharingGroup &, const Job &, double)> m_SharingPolicy;

    FatTreeResource m_Resources;
    std::vector<std::unique_ptr<Job>> m_RunningJobs;
    std::vector<std::unique_ptr<SharingGroup>> m_SharingGroups;
    std::unique_ptr<Job> m_NextJob;

    void BuildSharingGroups();
    void RunNewJobs(bool rebuildSharingGroups);
    // Returns the time of the next event, the job that will run next, and the sharing group of that job.
    std::tuple<double, Job *, SharingGroup *> GetNextEvent(double now);

public:
    explicit AllocationController(FatTreeResource &&resources, decltype(m_GetNextJob) &&getNextJob,
                                  decltype(m_HostAllocationPolicy) &&hostAllocationPolicy,
                                  decltype(m_TreeBuildingPolicy) &&treeBuildingPolicy,
                                  decltype(m_SharingPolicy) &&sharingPolicy);

    void RunSimulation();
};

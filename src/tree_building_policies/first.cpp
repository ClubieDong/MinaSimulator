#include "first.hpp"

void FirstTreeBuildingPolicy::operator()(const FatTreeResource &resources, const std::vector<std::unique_ptr<Job>> &,
                                         const std::vector<Job *> &newJobs) const {
    for (auto job : newJobs)
        for (auto root : resources.Topology->GetClosestCommonAncestors(job->GetHosts())) {
            auto tree = resources.Topology->GetAggregationTree(job->GetHosts(), root);
            if (!resources.CheckTreeConflict(tree)) {
                job->SetNextAggrTree(std::move(tree));
                break;
            }
        }
}

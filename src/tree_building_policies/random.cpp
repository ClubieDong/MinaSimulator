#include "random.hpp"
#include <random>

void RandomTreeBuildingPolicy::operator()(const FatTreeResource &resources, const std::vector<std::unique_ptr<Job>> &,
                                          const std::vector<Job *> &newJobs) const {
    for (auto job : newJobs) {
        std::vector<AggrTree> trees;
        for (auto root : resources.Topology->GetClosestCommonAncestors(job->GetHosts())) {
            auto tree = resources.Topology->GetAggregationTree(job->GetHosts(), root);
            if (!resources.CheckTreeConflict(tree))
                trees.push_back(std::move(tree));
        }
        if (!trees.empty()) {
            thread_local std::default_random_engine engine(std::random_device{}());
            std::uniform_int_distribution<std::size_t> random(0, trees.size() - 1);
            job->SetNextAggrTree(std::move(trees[random(engine)]));
        }
    }
}

#include "random.hpp"
#include <random>

void RandomTreeBuildingPolicy::operator()(const FatTreeResource &resources,
                                          const std::vector<std::unique_ptr<Job>> &jobs,
                                          const std::vector<Job *> &newJobs) const {
    std::vector<FatTree::AggrTree> newAggrTrees;
    for (auto newJob : newJobs) {
        std::vector<FatTree::AggrTree> availableTrees;
        for (auto root : resources.Topology->GetClosestCommonAncestors(newJob->GetHosts())) {
            auto currTree = resources.Topology->GetAggregationTree(newJob->GetHosts(), root);
            if (m_CheckConflict) {
                if (resources.CheckTreeConflict(currTree))
                    continue;
                bool conflict = false;
                for (const auto &tree : newAggrTrees)
                    if (resources.CheckTreeConflict(currTree, tree)) {
                        conflict = true;
                        break;
                    }
                if (conflict)
                    continue;
                for (const auto &job : jobs)
                    if (job->GetNextAggrTree() && resources.CheckTreeConflict(currTree, *job->GetNextAggrTree())) {
                        conflict = true;
                        break;
                    }
                if (conflict)
                    continue;
            }
            availableTrees.push_back(std::move(currTree));
            break;
        }
        if (!availableTrees.empty()) {
            thread_local std::default_random_engine engine(42);
            std::uniform_int_distribution<std::size_t> random(0, availableTrees.size() - 1);
            auto &chosenTree = availableTrees[random(engine)];
            newJob->SetNextAggrTree(chosenTree);
            newAggrTrees.push_back(std::move(chosenTree));
        }
    }
}

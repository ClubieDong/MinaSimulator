#include "first.hpp"

void FirstTreeBuildingPolicy::operator()(const FatTreeResource &resources,
                                         const std::vector<std::unique_ptr<Job>> &jobs,
                                         const std::vector<Job *> &newJobs) const {
    std::vector<FatTree::AggrTree> newAggrTrees;
    for (auto newJob : newJobs)
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
            newJob->SetNextAggrTree(currTree);
            newAggrTrees.push_back(std::move(currTree));
            break;
        }
}

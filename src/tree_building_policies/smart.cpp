#include "smart.hpp"
#include "utils/graph.hpp"
#include <cassert>
#include <random>

void SmartTreeBuildingPolicy::operator()(const FatTreeResource &resources,
                                         const std::vector<std::unique_ptr<Job>> &jobs,
                                         const std::vector<Job *> &) const {
    // Build aggregation tree for each job
    std::vector<FatTree::AggrTree> aggrTrees;
    std::vector<unsigned int> treeIdxToJobIdx;
    for (unsigned int jobIdx = 0; jobIdx < jobs.size(); ++jobIdx) {
        auto roots = resources.Topology->GetClosestCommonAncestors(jobs[jobIdx]->GetHosts());
        std::vector<const FatTree::Node *> chosenRoots;
        if (MaxTreeCount && *MaxTreeCount < roots.size()) {
            thread_local std::default_random_engine engine(std::random_device{}());
            std::sample(roots.cbegin(), roots.cend(), std::back_inserter(chosenRoots), *MaxTreeCount, engine);
        } else
            chosenRoots = roots;
        for (auto root : chosenRoots) {
            aggrTrees.push_back(resources.Topology->GetAggregationTree(jobs[jobIdx]->GetHosts(), root));
            treeIdxToJobIdx.push_back(jobIdx);
        }
    }
    // Build conflict graph
    Graph graph(aggrTrees.size());
    for (unsigned int i = 0; i < graph.NodeCount; ++i)
        graph.SetNodeWeight(i, jobs[treeIdxToJobIdx[i]]->HostCount);
    for (unsigned int i = 0; i < graph.NodeCount; ++i)
        for (unsigned int j = 0; j < graph.NodeCount; ++j) {
            if (i == j)
                continue;
            if (treeIdxToJobIdx[i] == treeIdxToJobIdx[j] || resources.CheckTreeConflict(aggrTrees[i], aggrTrees[j]))
                graph.AddEdge(i, j);
        }
    // Find maximum independent set
    auto mis = graph.CalcMaxIndependentSet();
    // Find sharing opportunities
    std::unordered_set<unsigned int> jobsInMis;
    for (auto center : mis)
        jobsInMis.insert(treeIdxToJobIdx[center]);
    std::vector<unsigned int> misNeighborCount(graph.NodeCount, 0);
    for (unsigned int center = 0; center < graph.NodeCount; ++center) {
        if (jobsInMis.count(treeIdxToJobIdx[center]) > 0)
            continue;
        for (auto i : graph.GetNeighbors(center))
            if (mis.count(i) > 0)
                ++misNeighborCount[i];
    }
    for (auto center : mis) {
        jobs[treeIdxToJobIdx[center]]->SetNextAggrTree(aggrTrees[center]);
        const auto &neighbors = graph.GetNeighbors(center);
        std::vector<char> availables(neighbors.size(), false);
        for (unsigned int i = 0; i < neighbors.size(); ++i)
            if (misNeighborCount[neighbors[i]] == 1)
                availables[i] = true;
        while (true) {
            unsigned int maxHostCount = 0, maxNeighborIdx;
            for (unsigned int i = 0; i < neighbors.size(); ++i)
                if (availables[i] && jobs[treeIdxToJobIdx[neighbors[i]]]->HostCount > maxHostCount) {
                    maxHostCount = jobs[treeIdxToJobIdx[neighbors[i]]]->HostCount;
                    maxNeighborIdx = i;
                }
            if (maxHostCount == 0)
                break;
            jobs[treeIdxToJobIdx[neighbors[maxNeighborIdx]]]->SetNextAggrTree(aggrTrees[neighbors[maxNeighborIdx]]);
            availables[maxNeighborIdx] = false;
            for (unsigned int i = 0; i < neighbors.size(); ++i)
                if (availables[i] && graph.HasEdge(neighbors[maxNeighborIdx], neighbors[i]))
                    availables[i] = false;
        }
    }
}

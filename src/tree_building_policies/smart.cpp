#include "smart.hpp"
#include "utils/graph.hpp"
#include <algorithm>
#include <cassert>
#include <random>

void SmartTreeBuildingPolicy::operator()(const FatTreeResource &resources,
                                         const std::vector<std::unique_ptr<Job>> &jobs, const std::vector<Job *> &) {
    // Reset all jobs' aggregation tree
    for (auto &job : jobs)
        job->SetNextAggrTree(std::nullopt);

    // Build aggregation tree for each job
    std::vector<const FatTree::AggrTree *> allAggrTrees;
    std::vector<unsigned int> treeIdxToJobIdx;
    for (unsigned int jobIdx = 0; jobIdx < jobs.size(); ++jobIdx) {
        auto job = jobs[jobIdx].get();
        if (m_JobToTrees.count(job->ID) == 0) {
            auto roots = resources.Topology->GetClosestCommonAncestors(job->GetHosts());
            std::vector<const FatTree::Node *> chosenRoots;
            if (MaxTreeCount && *MaxTreeCount < roots.size()) {
                thread_local std::default_random_engine engine(42);
                std::sample(roots.cbegin(), roots.cend(), std::back_inserter(chosenRoots), *MaxTreeCount, engine);
            } else
                chosenRoots = std::move(roots);
            auto &aggrTrees = m_JobToTrees[job->ID];
            for (auto root : chosenRoots)
                aggrTrees.push_back(resources.Topology->GetAggregationTree(job->GetHosts(), root));
        }
        for (auto &tree : m_JobToTrees[job->ID]) {
            allAggrTrees.push_back(&tree);
            treeIdxToJobIdx.push_back(jobIdx);
        }
    }

    // Build conflict graph
    Graph graph(allAggrTrees.size());
    for (unsigned int i = 0; i < graph.NodeCount; ++i)
        graph.SetNodeWeight(i, jobs[treeIdxToJobIdx[i]]->HostCount);
    for (unsigned int i = 0; i < graph.NodeCount; ++i)
        for (unsigned int j = 0; j < graph.NodeCount; ++j) {
            if (i == j)
                continue;
            if (treeIdxToJobIdx[i] == treeIdxToJobIdx[j] ||
                resources.CheckTreeConflict(*allAggrTrees[i], *allAggrTrees[j]))
                graph.AddEdge(i, j);
        }

    // Find maximum independent set
    auto mis = graph.CalcMaxIndependentSet();

    // Find sharing opportunities
    std::unordered_set<unsigned int> jobsInMis;
    for (auto center : mis)
        jobsInMis.insert(treeIdxToJobIdx[center]);
    std::vector<unsigned int> misNeighborCount(graph.NodeCount, 0);
    for (auto center : mis)
        for (auto i : graph.GetNeighbors(center))
            ++misNeighborCount[i];
    for (auto center : mis) {
        jobs[treeIdxToJobIdx[center]]->SetNextAggrTree(*allAggrTrees[center]);
        const auto &neighbors = graph.GetNeighbors(center);
        std::vector<char> availables(neighbors.size(), false);
        for (unsigned int i = 0; i < neighbors.size(); ++i)
            if (misNeighborCount[neighbors[i]] == 1 && treeIdxToJobIdx[neighbors[i]] != treeIdxToJobIdx[center])
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
            jobs[treeIdxToJobIdx[neighbors[maxNeighborIdx]]]->SetNextAggrTree(*allAggrTrees[neighbors[maxNeighborIdx]]);
            availables[maxNeighborIdx] = false;
            for (unsigned int i = 0; i < neighbors.size(); ++i)
                if (availables[i] && treeIdxToJobIdx[neighbors[maxNeighborIdx]] == treeIdxToJobIdx[neighbors[i]])
                    availables[i] = false;
        }
    }
}

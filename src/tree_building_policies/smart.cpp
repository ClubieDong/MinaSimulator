#include "smart.hpp"
#include "utils/graph.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <unordered_set>

constexpr bool SHOW_TIME = false;

void SmartTreeBuildingPolicy::operator()(const FatTreeResource &resources,
                                         const std::vector<std::unique_ptr<Job>> &jobs,
                                         const std::vector<Job *> &newJobs) {
    // Get sub graph from previous conflict graph
    auto start = std::chrono::high_resolution_clock::now();
    std::unordered_map<unsigned int, Job *> jobIdMap;
    for (auto &job : jobs)
        jobIdMap[job->ID] = job.get();
    std::vector<char> nodeSet(m_ConflictGraph.GetNodeCount(), false);
    std::vector<FatTree::AggrTree> newAggrTrees;
    std::vector<unsigned int> newTreeIdxToJobId;
    for (unsigned int i = 0; i < nodeSet.size(); ++i)
        if (jobIdMap.count(m_TreeIdxToJobId[i]) > 0) {
            nodeSet[i] = true;
            newAggrTrees.push_back(std::move(m_AggrTrees[i]));
            newTreeIdxToJobId.push_back(m_TreeIdxToJobId[i]);
        }
    m_ConflictGraph = m_ConflictGraph.GetSubGraph(nodeSet);
    m_AggrTrees = std::move(newAggrTrees);
    m_TreeIdxToJobId = std::move(newTreeIdxToJobId);
    auto finish = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count();
    if (SHOW_TIME)
        std::cout << "Sub graph time: " << duration / 1000.0 << " ms\n";

    // Build aggregation tree for each new job
    start = std::chrono::high_resolution_clock::now();
    for (auto newJob : newJobs) {
        auto roots = resources.Topology->GetClosestCommonAncestors(newJob->GetHosts());
        std::vector<const FatTree::Node *> chosenRoots;
        if (MaxTreeCount && *MaxTreeCount < roots.size()) {
            thread_local std::default_random_engine engine(42);
            std::sample(roots.cbegin(), roots.cend(), std::back_inserter(chosenRoots), *MaxTreeCount, engine);
        } else
            chosenRoots = std::move(roots);
        for (auto root : chosenRoots) {
            m_AggrTrees.push_back(resources.Topology->GetAggregationTree(newJob->GetHosts(), root));
            m_TreeIdxToJobId.push_back(newJob->ID);
        }
    }
    finish = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count();
    if (SHOW_TIME)
        std::cout << "Build aggregation tree time: " << duration / 1000.0 << " ms\n";

    // Build conflict graph incrementally
    start = std::chrono::high_resolution_clock::now();
    auto oldSize = m_ConflictGraph.GetNodeCount();
    m_ConflictGraph.SetNodeCount(m_AggrTrees.size());
    for (unsigned int i = oldSize; i < m_ConflictGraph.GetNodeCount(); ++i)
        m_ConflictGraph.SetNodeWeight(i, jobIdMap[m_TreeIdxToJobId[i]]->HostCount);
    for (unsigned int i = oldSize; i < m_ConflictGraph.GetNodeCount(); ++i)
        for (unsigned int j = 0; j < i; ++j)
            if (m_TreeIdxToJobId[i] == m_TreeIdxToJobId[j] ||
                resources.CheckTreeConflict(m_AggrTrees[i], m_AggrTrees[j]))
                m_ConflictGraph.AddEdge(i, j, false);
    finish = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count();
    if (SHOW_TIME)
        std::cout << "Incremental conflict graph time: " << duration / 1000.0 << " ms\n";

    // Find maximum independent set
    start = std::chrono::high_resolution_clock::now();
    auto mis = m_ConflictGraph.CalcMaxIndependentSet();
    finish = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count();
    if (SHOW_TIME)
        std::cout << "MIS time: " << duration / 1000.0 << " ms\n";

    // Find sharing opportunities
    start = std::chrono::high_resolution_clock::now();
    std::vector<unsigned int> misNeighborCount(m_ConflictGraph.GetNodeCount(), 0);
    for (auto center : mis)
        for (auto i : m_ConflictGraph.GetNeighbors(center))
            ++misNeighborCount[i];
    for (auto &job : jobs)
        job->SetNextAggrTree(std::nullopt);
    std::unordered_set<unsigned int> addedJobIds;
    for (auto center : mis) {
        jobIdMap[m_TreeIdxToJobId[center]]->SetNextAggrTree(m_AggrTrees[center]);
        addedJobIds.insert(m_TreeIdxToJobId[center]);
        for (auto neighbor : m_ConflictGraph.GetNeighbors(center)) {
            auto jobId = m_TreeIdxToJobId[neighbor];
            if (misNeighborCount[neighbor] != 1 || addedJobIds.count(jobId) > 0)
                continue;
            jobIdMap[jobId]->SetNextAggrTree(m_AggrTrees[neighbor]);
            addedJobIds.insert(jobId);
        }
    }
    finish = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count();
    if (SHOW_TIME)
        std::cout << "Find sharing opportunities time: " << duration / 1000.0 << " ms\n";

    if (SHOW_TIME)
        std::cout << '\n';
}

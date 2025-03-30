#include "smart.hpp"
#include "utils/graph.hpp"
#include <algorithm>
#include <cassert>
#include <random>

void SmartTreeBuildingPolicy::operator()(const FatTreeResource &resources,
                                         const std::vector<std::unique_ptr<Job>> &jobs,
                                         const std::vector<Job *> &newJobs) {
    // Get sub graph from previous conflict graph
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

    // Build aggregation tree for each new job
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

    // Build conflict graph incrementally
    auto oldSize = m_ConflictGraph.GetNodeCount();
    m_ConflictGraph.SetNodeCount(m_AggrTrees.size());
    for (unsigned int i = oldSize; i < m_ConflictGraph.GetNodeCount(); ++i)
        m_ConflictGraph.SetNodeWeight(i, jobIdMap[m_TreeIdxToJobId[i]]->HostCount);
    for (unsigned int i = oldSize; i < m_ConflictGraph.GetNodeCount(); ++i)
        for (unsigned int j = 0; j < i; ++j)
            if (m_TreeIdxToJobId[i] == m_TreeIdxToJobId[j] ||
                resources.CheckTreeConflict(m_AggrTrees[i], m_AggrTrees[j]))
                m_ConflictGraph.AddEdge(i, j, false);

    // Find maximum independent set
    auto mis = m_ConflictGraph.CalcMaxIndependentSet();

    // Find sharing opportunities
    std::unordered_set<unsigned int> jobsInMis;
    for (auto center : mis)
        jobsInMis.insert(m_TreeIdxToJobId[center]);
    std::vector<unsigned int> misNeighborCount(m_ConflictGraph.GetNodeCount(), 0);
    for (auto center : mis)
        for (auto i : m_ConflictGraph.GetNeighbors(center))
            ++misNeighborCount[i];
    for (auto &job : jobs)
        job->SetNextAggrTree(std::nullopt);
    for (auto center : mis) {
        jobIdMap[m_TreeIdxToJobId[center]]->SetNextAggrTree(m_AggrTrees[center]);
        const auto &neighbors = m_ConflictGraph.GetNeighbors(center);
        std::vector<char> availables(neighbors.size(), false);
        for (unsigned int i = 0; i < neighbors.size(); ++i)
            if (misNeighborCount[neighbors[i]] == 1 && m_TreeIdxToJobId[neighbors[i]] != m_TreeIdxToJobId[center])
                availables[i] = true;
        while (true) {
            unsigned int maxHostCount = 0, maxNeighborIdx;
            for (unsigned int i = 0; i < neighbors.size(); ++i)
                if (availables[i] && jobIdMap[m_TreeIdxToJobId[neighbors[i]]]->HostCount > maxHostCount) {
                    maxHostCount = jobIdMap[m_TreeIdxToJobId[neighbors[i]]]->HostCount;
                    maxNeighborIdx = i;
                }
            if (maxHostCount == 0)
                break;
            jobIdMap[m_TreeIdxToJobId[neighbors[maxNeighborIdx]]]->SetNextAggrTree(
                m_AggrTrees[neighbors[maxNeighborIdx]]);
            availables[maxNeighborIdx] = false;
            for (unsigned int i = 0; i < neighbors.size(); ++i)
                if (availables[i] && m_TreeIdxToJobId[neighbors[maxNeighborIdx]] == m_TreeIdxToJobId[neighbors[i]])
                    availables[i] = false;
        }
    }
}

#include "graph.hpp"
#include "mis_solver.hpp"
#include <algorithm>
#include <cassert>

unsigned int Graph::SortAdjacencyList() {
    unsigned int edgeCount = 0;
    for (auto &list : m_AdjacencyList) {
        std::sort(list.begin(), list.end());
        list.erase(std::unique(list.begin(), list.end()), list.end());
        edgeCount += list.size();
    }
    assert(edgeCount % 2 == 0);
    return edgeCount / 2;
}

void Graph::SetNodeCount(unsigned int nodeCount) {
    assert(nodeCount >= m_NodeCount);
    m_NodeWeights.resize(nodeCount, 1);
    m_AdjacencyList.resize(nodeCount);
    m_NodeCount = nodeCount;
}

const std::vector<unsigned int> &Graph::GetNeighbors(unsigned int node) const {
    assert(node < m_NodeCount);
    return m_AdjacencyList[node];
}

bool Graph::HasEdge(unsigned int node1, unsigned int node2) const {
    assert(node1 < m_NodeCount);
    assert(node2 < m_NodeCount);
    const auto &list = m_AdjacencyList[node1];
    return std::find(list.cbegin(), list.cend(), node2) != list.cend();
}

void Graph::SetNodeWeight(unsigned int node, unsigned int weight) {
    assert(node < m_NodeCount);
    m_NodeWeights[node] = weight;
}

void Graph::AddEdge(unsigned int node1, unsigned int node2, bool directed) {
    assert(node1 != node2);
    assert(node1 < m_NodeCount);
    assert(node2 < m_NodeCount);
    m_AdjacencyList[node1].push_back(node2);
    if (!directed)
        m_AdjacencyList[node2].push_back(node1);
}

Graph Graph::GetSubGraph(const std::vector<char> &nodeSet) const {
    assert(nodeSet.size() == m_NodeCount);
    std::vector<unsigned int> oldToNew(m_NodeCount, -1u);
    unsigned int newNodeCount = 0;
    for (unsigned int i = 0; i < nodeSet.size(); ++i)
        if (nodeSet[i]) {
            oldToNew[i] = newNodeCount;
            ++newNodeCount;
        }
    Graph subGraph;
    subGraph.SetNodeCount(newNodeCount);
    for (unsigned int i = 0; i < nodeSet.size(); ++i) {
        if (!nodeSet[i])
            continue;
        subGraph.m_NodeWeights[oldToNew[i]] = m_NodeWeights[i];
        for (auto neighbor : m_AdjacencyList[i])
            if (oldToNew[neighbor] != -1u)
                subGraph.m_AdjacencyList[oldToNew[i]].push_back(oldToNew[neighbor]);
    }
    return subGraph;
}

std::unordered_set<unsigned int> Graph::CalcMaxIndependentSet() {
    auto edgeCount = SortAdjacencyList();
    std::vector<unsigned int> nodeOffsets, edges;
    nodeOffsets.reserve(m_NodeCount + 1);
    edges.reserve(edgeCount);
    nodeOffsets.push_back(0);
    for (const auto &adj : m_AdjacencyList) {
        std::copy(adj.cbegin(), adj.cend(), std::back_inserter(edges));
        nodeOffsets.push_back(nodeOffsets.back() + adj.size());
    }
    MisSolver solver(m_NodeCount, edgeCount, std::move(nodeOffsets), std::move(edges));
    auto inMis = solver.LinearSolver();
    std::unordered_set<unsigned int> mis;
    for (unsigned int i = 0; i < inMis.size(); ++i)
        if (inMis[i])
            mis.insert(i);
    return mis;
}

#pragma once

#include <unordered_set>
#include <vector>

class Graph {
private:
    unsigned int m_NodeCount = 0;
    std::vector<unsigned int> m_NodeWeights;
    std::vector<std::vector<unsigned int>> m_AdjacencyList;

    // Sort and deduplicate the adjacency list, returns the number of edges.
    unsigned int SortAdjacencyList();

public:
    void SetNodeCount(unsigned int nodeCount);
    unsigned int GetNodeCount() const { return m_NodeCount; }
    const std::vector<unsigned int> &GetNeighbors(unsigned int node) const;
    bool HasEdge(unsigned int node1, unsigned int node2) const;
    void SetNodeWeight(unsigned int node, unsigned int weight);
    void AddEdge(unsigned int node1, unsigned int node2, bool directed);
    Graph GetSubGraph(const std::vector<char> &nodeSet) const;
    std::unordered_set<unsigned int> CalcMaxIndependentSet();
};

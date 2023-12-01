#pragma once

#include <vector>
#include <unordered_set>

class Graph {
private:
    std::vector<unsigned int> m_NodeWeights;
    std::vector<std::vector<unsigned int>> m_AdjacencyList;

    // Sort and deduplicate the adjacency list, returns the number of edges.
    unsigned int SortAdjacencyList();

public:
    inline static double MisSolverTimeLimit = 1.0; // In second

    const unsigned int NodeCount;

    explicit Graph(unsigned int nodeCount)
        : m_NodeWeights(nodeCount, 1), m_AdjacencyList(nodeCount), NodeCount(nodeCount) {}

    const std::vector<unsigned int> &GetNeighbors(unsigned int node) const;
    bool HasEdge(unsigned int node1, unsigned int node2) const;
    void SetNodeWeight(unsigned int node, unsigned int weight);
    void AddEdge(unsigned int node1, unsigned int node2);

    std::unordered_set<unsigned int> CalcMaxIndependentSet();
};

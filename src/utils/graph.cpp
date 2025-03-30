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

const std::vector<unsigned int> &Graph::GetNeighbors(unsigned int node) const {
    assert(node < NodeCount);
    return m_AdjacencyList[node];
}

bool Graph::HasEdge(unsigned int node1, unsigned int node2) const {
    assert(node1 < NodeCount);
    assert(node2 < NodeCount);
    const auto &list = m_AdjacencyList[node1];
    return std::find(list.cbegin(), list.cend(), node2) != list.cend();
}

void Graph::SetNodeWeight(unsigned int node, unsigned int weight) {
    assert(node < NodeCount);
    m_NodeWeights[node] = weight;
}

void Graph::AddEdge(unsigned int node1, unsigned int node2) {
    assert(node1 < NodeCount);
    assert(node2 < NodeCount);
    m_AdjacencyList[node1].push_back(node2);
    m_AdjacencyList[node2].push_back(node1);
}

std::unordered_set<unsigned int> Graph::CalcMaxIndependentSet() {
    auto edgeCount = SortAdjacencyList();
    std::vector<unsigned int> nodeOffsets, edges;
    nodeOffsets.reserve(NodeCount + 1);
    edges.reserve(edgeCount);
    nodeOffsets.push_back(0);
    for (const auto &adj : m_AdjacencyList) {
        std::copy(adj.cbegin(), adj.cend(), std::back_inserter(edges));
        nodeOffsets.push_back(nodeOffsets.back() + adj.size());
    }
    MisSolver solver(NodeCount, edgeCount, std::move(nodeOffsets), std::move(edges));
    auto inMis = solver.NearLinearSolver();
    std::unordered_set<unsigned int> mis;
    for (unsigned int i = 0; i < inMis.size(); ++i)
        if (inMis[i])
            mis.insert(i);
    return mis;
}

#if false
// This implementation is based on KaMIS (https://github.com/KarlsruheMIS/KaMIS)
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>

std::unordered_set<unsigned int> Graph::CalcMaxIndependentSet() {
    thread_local std::string misSolverPath = "./mis_solver";
    thread_local std::string misInputPath = "input.txt";
    thread_local std::string misOutputPath = "output.txt";
    thread_local double misSolverTimeLimit = 1.0;
    // Prepare input file
    auto edgeCount = SortAdjacencyList();
    std::ofstream inputFile(misInputPath);
    inputFile << NodeCount << ' ' << edgeCount << " 10\n";
    for (unsigned int i = 0; i < NodeCount; ++i) {
        inputFile << m_NodeWeights[i] << ' ';
        for (auto node : m_AdjacencyList[i])
            inputFile << node + 1 << ' ';
        inputFile << '\n';
    }
    inputFile.close();
    // Run MIS solver
    auto command = misSolverPath + " " + misInputPath + " --output=" + misOutputPath +
                   " --time_limit=" + std::to_string(misSolverTimeLimit) + " --disable_checks > /dev/null";
    auto result = std::system(command.c_str());
    if (result != 0)
        throw std::runtime_error("MIS solver failed!");
    // Parse output file
    std::unordered_set<unsigned int> mis;
    std::ifstream outputFile(misOutputPath);
    for (unsigned int i = 0; i < NodeCount; ++i) {
        std::string line;
        std::getline(outputFile, line);
        assert(line.size() == 1);
        assert(line[0] == '0' || line[0] == '1');
        if (line[0] == '1')
            mis.insert(i);
    }
    outputFile.close();
    return mis;
}
#endif

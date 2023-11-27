#include "graph.hpp"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <string>

static std::string MisSolverPath = "./mis_solver";
static std::string MisInputPath = "input.txt";
static std::string MisOutputPath = "output.txt";

unsigned int Graph::SortAdjacencyList() {
    unsigned int edgeCount = 0;
    for (auto &list : m_AdjacencyList) {
        std::sort(list.begin(), list.end());
        list.erase(std::unique(list.begin(), list.end()), list.end());
        edgeCount += list.size();
    }
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
    // Prepare input file
    auto edgeCount = SortAdjacencyList();
    std::ofstream inputFile(MisInputPath);
    inputFile << NodeCount << ' ' << edgeCount << " 10\n";
    for (unsigned int i = 0; i < NodeCount; ++i) {
        inputFile << m_NodeWeights[i] << ' ';
        for (auto node : m_AdjacencyList[i])
            inputFile << node + 1 << ' ';
        inputFile << '\n';
    }
    inputFile.close();
    // Run MIS solver
    auto command = MisSolverPath + " " + MisInputPath + " --output=" + MisOutputPath +
                   " --time_limit=1 --disable_checks > /dev/null";
    auto result = std::system(command.c_str());
    if (result != 0)
        throw std::runtime_error("MIS solver failed!");
    // Parse output file
    std::unordered_set<unsigned int> mis;
    std::ifstream outputFile(MisOutputPath);
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

#pragma once

#include <utility>
#include <vector>

class MisSolver {
private:
    unsigned int m_NodeCount;
    unsigned int m_EdgeCount;
    std::vector<unsigned int> m_NodeOffsets; // Offset of neighbors of nodes
    std::vector<unsigned int> m_Edges;       // Adjacent ids of edges

    void Shrink(unsigned int u, unsigned int &end, const std::vector<char> &is);
    void Shrink(unsigned int u, unsigned int &end, const std::vector<char> &is,
                std::vector<unsigned int>::iterator tri);
    void DeleteVertex(unsigned int v, std::vector<char> &is, std::vector<int> &degree,
                      std::vector<unsigned int> &degreeOnes, std::vector<unsigned int> &degreeTwos);
    void DeleteVertex(unsigned int v, const std::vector<unsigned int> &pend, std::vector<char> &is,
                      std::vector<int> &degree, std::vector<unsigned int> &degreeOnes,
                      std::vector<unsigned int> &degreeTwos);
    void DeleteVertex(unsigned int u, std::vector<unsigned int> &pend, std::vector<char> &is,
                      std::vector<unsigned int> &degreeTwos, std::vector<unsigned int>::iterator tri,
                      std::vector<char> &adj, std::vector<int> &degree, std::vector<char> &dominate,
                      std::vector<unsigned int> &dominated);
    bool ExistEdge(unsigned int u1, unsigned int u2);
    bool ExistEdge(unsigned int u, unsigned int v, const std::vector<unsigned int> &pend);
    void EdgeRewire(unsigned int u, unsigned int u1, unsigned int u2);
    void EdgeRewrite(unsigned int u, const std::vector<unsigned int> &pend, unsigned int v, unsigned int w);
    void InitialDominanceAndDegreeTwoRemove(std::vector<unsigned int> &degreeOnes,
                                            std::vector<unsigned int> &degreeTwos, std::vector<char> &is,
                                            std::vector<int> &degree, std::vector<char> &adj,
                                            std::vector<std::pair<unsigned int, unsigned int>> &S);
    void RemoveDegreeOneTwo(std::vector<unsigned int> &degreeOnes, std::vector<unsigned int> &degreeTwos,
                            std::vector<char> &is, std::vector<int> &degree,
                            std::vector<std::pair<unsigned int, unsigned int>> &S);
    void GetTwoNeighbors(unsigned int u, const std::vector<char> &is, unsigned int &u1, unsigned int &u2);
    unsigned int GetOtherNeighbor(unsigned int u, const std::vector<char> &is, unsigned int u1);
    void LpReduction(const std::vector<unsigned int> &ids, unsigned int idsN, std::vector<char> &is,
                     std::vector<int> &degree);
    void ComputeTriangleCounts(std::vector<unsigned int>::iterator tri, std::vector<unsigned int> &pend,
                               std::vector<char> &adj, std::vector<char> &is, std::vector<int> &degree,
                               std::vector<char> &dominate, std::vector<unsigned int> &dominated);
    bool DominatedCheck(unsigned int u, const std::vector<unsigned int> &pend, const std::vector<char> &is,
                        std::vector<unsigned int>::const_iterator tri, const std::vector<int> &degree);
    void UpdateTriangle(unsigned int u1, unsigned int u2, std::vector<unsigned int> &pend, std::vector<char> &is,
                        std::vector<char> &adj, std::vector<unsigned int>::iterator tri, const std::vector<int> &degree,
                        std::vector<char> &dominate, std::vector<unsigned int> &dominated);

public:
    explicit MisSolver(unsigned int nodeCount, unsigned int edgeCount, std::vector<unsigned int> &&nodeOffsets,
                       std::vector<unsigned int> &&edges)
        : m_NodeCount(nodeCount), m_EdgeCount(edgeCount), m_NodeOffsets(std::move(nodeOffsets)),
          m_Edges(std::move(edges)) {}

    std::vector<char> LinearSolver();
    std::vector<char> NearLinearSolver();
};

#pragma once

#include <array>
#include <ostream>
#include <vector>

class FatTree {
public:
    static constexpr unsigned int Height = 3;

    class Node {
    public:
        const unsigned int ID;
        const unsigned int Layer;
        const std::array<unsigned int, Height> Indices;

        explicit Node(unsigned int id, unsigned int layer, const std::array<unsigned int, Height> &indices)
            : ID(id), Layer(layer), Indices(indices) {}

        friend std::ostream &operator<<(std::ostream &os, const Node &node);
    };

    class Edge {
    public:
        const unsigned int ID;
        const Node *const Parent;
        const Node *const Child;

        explicit Edge(unsigned int id, const Node *parent, const Node *child) : ID(id), Parent(parent), Child(child) {}

        friend std::ostream &operator<<(std::ostream &os, const Edge &edge);
    };

    class AggrTree {
    public:
        std::vector<const Node *> Nodes;
        std::vector<const Edge *> Edges;
        std::vector<char> NodeSet, EdgeSet;

        explicit AggrTree(std::vector<const Node *> &&nodes, std::vector<const Edge *> &&edges,
                          std::vector<char> &&nodeSet, std::vector<char> &&edgeSet)
            : Nodes(std::move(nodes)), Edges(std::move(edges)), NodeSet(std::move(nodeSet)),
              EdgeSet(std::move(edgeSet)) {}

        friend bool operator==(const AggrTree &tree1, const AggrTree &tree2);
        friend bool operator!=(const AggrTree &tree1, const AggrTree &tree2);
    };

public:
    const std::array<unsigned int, Height> DownLinkCount;
    const std::array<unsigned int, Height> UpLinkCount;
    const std::vector<Node> Nodes;
    const std::array<std::vector<const Node *>, Height + 1> NodesByLayer;
    const std::vector<Edge> Edges;
    const std::array<std::vector<const Edge *>, Height> EdgesByLayer;

    explicit FatTree(const std::array<unsigned int, Height> &downLinkCount,
                     const std::array<unsigned int, Height> &upLinkCount);
    explicit FatTree(unsigned int degree);

    unsigned int GetNodeID(unsigned int layer, const std::array<unsigned int, Height> &indices) const;
    unsigned int GetEdgeID(const Node *parent, const Node *child) const;

    std::vector<const Node *> GetClosestCommonAncestors(const std::vector<const Node *> &leaves) const;
    AggrTree GetAggregationTree(const std::vector<const Node *> &leaves, const Node *root) const;
};

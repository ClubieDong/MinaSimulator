#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <ostream>
#include <queue>
#include <unordered_set>
#include <vector>

template <unsigned int Height>
class FatTree {
    static_assert(Height >= 1);

public:
    class Node {
    public:
        const unsigned int ID;
        const unsigned int Layer;
        const std::array<unsigned int, Height> Indices;

        explicit Node(unsigned int id, unsigned int layer, const std::array<unsigned int, Height> &indices)
            : ID(id), Layer(layer), Indices(indices) {}

        friend std::ostream &operator<<(std::ostream &os, const Node &node) {
            os << "Node(ID=" << node.ID << ",Layer=" << node.Layer << ",Indices=";
            for (unsigned int i = 0; i < Height; ++i)
                os << (i == 0 ? '[' : ',') << node.Indices[i];
            os << "])";
            return os;
        }
    };

    class Edge {
    public:
        const unsigned int ID;
        const Node *const Parent;
        const Node *const Child;

        explicit Edge(unsigned int id, const Node *parent, const Node *child) : ID(id), Parent(parent), Child(child) {}

        friend std::ostream &operator<<(std::ostream &os, const Edge &edge) {
            os << "Edge(ID=" << edge.ID << ",Parent=" << *edge.Parent << ",Child=" << *edge.Child << ")";
            return os;
        }
    };

private:
    static std::array<unsigned int, Height> CreateDownLinkCountFromDegree(unsigned int degree) {
        assert(degree >= 2 && degree % 2 == 0);
        std::array<unsigned int, Height> downLinkCount;
        downLinkCount.fill(degree / 2);
        downLinkCount[Height - 1] = degree;
        return downLinkCount;
    }

    static std::array<unsigned int, Height> CreateUpLinkCountFromDegree(unsigned int degree) {
        assert(degree >= 2 && degree % 2 == 0);
        std::array<unsigned int, Height> upLinkCount;
        upLinkCount.fill(degree / 2);
        upLinkCount[0] = 1;
        return upLinkCount;
    }

    static std::vector<Node> CreateNodes(const std::array<unsigned int, Height> &downLinkCount,
                                         const std::array<unsigned int, Height> &upLinkCount) {
        std::vector<Node> nodes;
        for (unsigned int layer = 0; layer <= Height; ++layer) {
            std::array<unsigned int, Height> indices = {};
            while (true) {
                nodes.emplace_back(nodes.size(), layer, indices);
                unsigned int i = 0;
                for (; i < Height; ++i) {
                    ++indices[i];
                    if (indices[i] < (i < layer ? upLinkCount[i] : downLinkCount[i]))
                        break;
                    indices[i] = 0;
                }
                if (i == Height)
                    break;
            }
        }
        return nodes;
    }

    static std::array<std::vector<const Node *>, Height + 1> CreateNodesByLayer(const std::vector<Node> &nodes) {
        std::array<std::vector<const Node *>, Height + 1> nodesByLayer;
        for (const auto &node : nodes)
            nodesByLayer[node.Layer].push_back(&node);
        return nodesByLayer;
    }

    static std::vector<Edge> CreateEdges(const std::array<unsigned int, Height> &downLinkCount,
                                         const std::array<unsigned int, Height> &upLinkCount,
                                         const std::array<std::vector<const Node *>, Height + 1> &nodesByLayer) {
        std::vector<Edge> edges;
        for (unsigned int layer = 0; layer < Height; ++layer)
            for (auto child : nodesByLayer[layer]) {
                unsigned int radix = 1, firstParentId = 0, step;
                for (unsigned int i = 0; i < Height; ++i) {
                    if (i == layer)
                        step = radix;
                    else
                        firstParentId += radix * child->Indices[i];
                    radix *= i < layer + 1 ? upLinkCount[i] : downLinkCount[i];
                }
                for (unsigned int i = 0; i < upLinkCount[layer]; ++i) {
                    assert(firstParentId + i * step < nodesByLayer[layer + 1].size());
                    auto parent = nodesByLayer[layer + 1][firstParentId + i * step];
                    edges.emplace_back(edges.size(), parent, child);
                }
            }
        return edges;
    }

    static std::array<std::vector<const Edge *>, Height> CreateEdgesByLayer(const std::vector<Edge> &edges) {
        std::array<std::vector<const Edge *>, Height> edgesByLayer;
        for (const auto &edge : edges)
            edgesByLayer[edge.Child->Layer].push_back(&edge);
        return edgesByLayer;
    }

public:
    const std::array<unsigned int, Height> DownLinkCount;
    const std::array<unsigned int, Height> UpLinkCount;
    const std::vector<Node> Nodes;
    const std::array<std::vector<const Node *>, Height + 1> NodesByLayer;
    const std::vector<Edge> Edges;
    const std::array<std::vector<const Edge *>, Height> EdgesByLayer;

    explicit FatTree(const std::array<unsigned int, Height> &downLinkCount,
                     const std::array<unsigned int, Height> &upLinkCount)
        : DownLinkCount(downLinkCount), UpLinkCount(upLinkCount), Nodes(CreateNodes(DownLinkCount, UpLinkCount)),
          NodesByLayer(CreateNodesByLayer(Nodes)), Edges(CreateEdges(DownLinkCount, UpLinkCount, NodesByLayer)),
          EdgesByLayer(CreateEdgesByLayer(Edges)) {}

    explicit FatTree(unsigned int degree)
        : FatTree(CreateDownLinkCountFromDegree(degree), CreateUpLinkCountFromDegree(degree)) {}

    unsigned int GetNodeID(unsigned int layer, const std::array<unsigned int, Height> &indices) const {
        unsigned int id = 0;
        for (unsigned int i = 0; i < layer; ++i)
            id += NodesByLayer[i].size();
        unsigned int radix = 1;
        for (unsigned int i = 0; i < Height; ++i) {
            id += radix * indices[i];
            radix *= i < layer ? UpLinkCount[i] : DownLinkCount[i];
        }
        return id;
    }

    unsigned int GetEdgeID(const Node *parent, const Node *child) const {
        unsigned int id = 0;
        for (unsigned int i = 0; i < child->Layer; ++i)
            id += EdgesByLayer[i].size();
        unsigned int radix = UpLinkCount[child->Layer];
        for (unsigned int i = 0; i < Height; ++i) {
            id += radix * child->Indices[i];
            radix *= i < child->Layer ? UpLinkCount[i] : DownLinkCount[i];
        }
        return id + parent->Indices[child->Layer];
    }

    std::vector<const Node *> GetClosestCommonAncestors(const std::vector<const Node *> &leaves) const {
        assert(leaves.size() > 0);
        unsigned int ancestorLayer = 0;
        for (int i = Height - 1; i >= 0; --i)
            if (!std::all_of(leaves.cbegin() + 1, leaves.cend(), [i, &leaves](const Node *leaf) {
                    return leaf->Indices[i] == leaves.front()->Indices[i];
                })) {
                ancestorLayer = i + 1;
                break;
            }
        unsigned int ancestorCount = 1;
        for (unsigned int i = 0; i < ancestorLayer; ++i)
            ancestorCount *= UpLinkCount[i];
        unsigned int firstAncestorId = 0, radix = ancestorCount;
        for (unsigned int i = ancestorLayer; i < Height; ++i) {
            firstAncestorId += radix * leaves.front()->Indices[i];
            radix *= DownLinkCount[i];
        }
        std::vector<const Node *> ancestors;
        ancestors.reserve(ancestorCount);
        for (unsigned int i = 0; i < ancestorCount; ++i)
            ancestors.push_back(NodesByLayer[ancestorLayer][firstAncestorId + i]);
        return ancestors;
    }

    std::pair<std::vector<const Node *>, std::vector<const Edge *>>
    GetAggregationTree(const std::vector<const Node *> &leaves, const Node *root) const {
        assert(leaves.size() > 0);
        std::unordered_set<const Node *> nodes(leaves.cbegin(), leaves.cend());
        std::vector<const Edge *> edges;
        std::queue<const Node *> queue;
        for (auto leaf : leaves)
            queue.push(leaf);
        while (!queue.empty()) {
            auto child = queue.front();
            queue.pop();
            if (child == root)
                continue;
            auto parentIndices = child->Indices;
            parentIndices[child->Layer] = root->Indices[child->Layer];
            auto parent = &Nodes[GetNodeID(child->Layer + 1, parentIndices)];
            edges.push_back(&Edges[GetEdgeID(parent, child)]);
            if (nodes.count(parent) == 0) {
                nodes.insert(parent);
                queue.push(parent);
            }
        }
        return {std::vector(nodes.cbegin(), nodes.cend()), std::move(edges)};
    }
};

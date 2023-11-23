#include "fat_tree.hpp"
#include <algorithm>
#include <cassert>
#include <queue>
#include <unordered_set>

std::ostream &operator<<(std::ostream &os, const FatTree::Node &node) {
    os << "Node(ID=" << node.ID << ",Layer=" << node.Layer << ",Indices=";
    for (unsigned int i = 0; i < FatTree::Height; ++i)
        os << (i == 0 ? '[' : ',') << node.Indices[i];
    os << "])";
    return os;
}

std::ostream &operator<<(std::ostream &os, const FatTree::Edge &edge) {
    os << "Edge(ID=" << edge.ID << ",Parent=" << *edge.Parent << ",Child=" << *edge.Child << ")";
    return os;
}

static std::array<unsigned int, FatTree::Height> CreateDownLinkCountFromDegree(unsigned int degree) {
    assert(degree >= 2);
    assert(degree % 2 == 0);
    std::array<unsigned int, FatTree::Height> downLinkCount;
    downLinkCount.fill(degree / 2);
    downLinkCount[FatTree::Height - 1] = degree;
    return downLinkCount;
}

static std::array<unsigned int, FatTree::Height> CreateUpLinkCountFromDegree(unsigned int degree) {
    assert(degree >= 2);
    assert(degree % 2 == 0);
    std::array<unsigned int, FatTree::Height> upLinkCount;
    upLinkCount.fill(degree / 2);
    upLinkCount[0] = 1;
    return upLinkCount;
}

static std::vector<FatTree::Node> CreateNodes(const std::array<unsigned int, FatTree::Height> &downLinkCount,
                                              const std::array<unsigned int, FatTree::Height> &upLinkCount) {
    std::vector<FatTree::Node> nodes;
    for (unsigned int layer = 0; layer <= FatTree::Height; ++layer) {
        std::array<unsigned int, FatTree::Height> indices = {};
        while (true) {
            nodes.emplace_back(nodes.size(), layer, indices);
            unsigned int i = 0;
            for (; i < FatTree::Height; ++i) {
                ++indices[i];
                if (indices[i] < (i < layer ? upLinkCount[i] : downLinkCount[i]))
                    break;
                indices[i] = 0;
            }
            if (i == FatTree::Height)
                break;
        }
    }
    return nodes;
}

static std::array<std::vector<const FatTree::Node *>, FatTree::Height + 1>
CreateNodesByLayer(const std::vector<FatTree::Node> &nodes) {
    std::array<std::vector<const FatTree::Node *>, FatTree::Height + 1> nodesByLayer;
    for (const auto &node : nodes)
        nodesByLayer[node.Layer].push_back(&node);
    return nodesByLayer;
}

static std::vector<FatTree::Edge>
CreateEdges(const std::array<unsigned int, FatTree::Height> &downLinkCount,
            const std::array<unsigned int, FatTree::Height> &upLinkCount,
            const std::array<std::vector<const FatTree::Node *>, FatTree::Height + 1> &nodesByLayer) {
    std::vector<FatTree::Edge> edges;
    for (unsigned int layer = 0; layer < FatTree::Height; ++layer)
        for (auto child : nodesByLayer[layer]) {
            unsigned int radix = 1, firstParentId = 0, step;
            for (unsigned int i = 0; i < FatTree::Height; ++i) {
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

static std::array<std::vector<const FatTree::Edge *>, FatTree::Height>
CreateEdgesByLayer(const std::vector<FatTree::Edge> &edges) {
    std::array<std::vector<const FatTree::Edge *>, FatTree::Height> edgesByLayer;
    for (const auto &edge : edges)
        edgesByLayer[edge.Child->Layer].push_back(&edge);
    return edgesByLayer;
}

FatTree::FatTree(const std::array<unsigned int, Height> &downLinkCount,
                 const std::array<unsigned int, Height> &upLinkCount)
    : DownLinkCount(downLinkCount), UpLinkCount(upLinkCount), Nodes(CreateNodes(DownLinkCount, UpLinkCount)),
      NodesByLayer(CreateNodesByLayer(Nodes)), Edges(CreateEdges(DownLinkCount, UpLinkCount, NodesByLayer)),
      EdgesByLayer(CreateEdgesByLayer(Edges)) {}

FatTree::FatTree(unsigned int degree)
    : FatTree(CreateDownLinkCountFromDegree(degree), CreateUpLinkCountFromDegree(degree)) {}

unsigned int FatTree::GetNodeID(unsigned int layer, const std::array<unsigned int, Height> &indices) const {
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

unsigned int FatTree::GetEdgeID(const Node *parent, const Node *child) const {
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

std::vector<const FatTree::Node *> FatTree::GetClosestCommonAncestors(const std::vector<const Node *> &leaves) const {
    assert(leaves.size() > 0);
    unsigned int ancestorLayer = 0;
    for (int i = Height - 1; i >= 0; --i)
        if (!std::all_of(leaves.cbegin() + 1, leaves.cend(),
                         [i, &leaves](const Node *leaf) { return leaf->Indices[i] == leaves.front()->Indices[i]; })) {
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

FatTree::AggrTree FatTree::GetAggregationTree(const std::vector<const Node *> &leaves, const Node *root) const {
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

#pragma once

#include <array>
#include <cassert>
#include <ostream>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

template <unsigned int Height>
class FatTree {
    static_assert(Height >= 1);

private:
    void Initialize() {
        // Initialize NodesByLayer
        for (unsigned int layer = 0; layer <= Height; ++layer) {
            std::array<unsigned int, Height> indices = {};
            while (true) {
                NodesByLayer[layer].push_back({layer, indices});
                unsigned int i = 0;
                for (; i < Height; ++i) {
                    ++indices[i];
                    if (indices[i] < (i < layer ? UpLinkCount[i] : DownLinkCount[i]))
                        break;
                    indices[i] = 0;
                }
                if (i == Height)
                    break;
            }
        }
        // Initialize Parents and Children
        for (const auto &layer : NodesByLayer)
            for (auto node : layer) {
                auto &parents = Parents[node], &children = Children[node];
                auto [layer, indices] = node;
                if (layer < Height)
                    for (unsigned int i = 0; i < UpLinkCount[layer]; ++i) {
                        auto parentIndices = indices;
                        parentIndices[layer] = i;
                        parents.push_back({layer + 1, parentIndices});
                    }
                if (layer > 0)
                    for (unsigned int i = 0; i < DownLinkCount[layer - 1]; ++i) {
                        auto childIndices = indices;
                        childIndices[layer - 1] = i;
                        children.push_back({layer - 1, childIndices});
                    }
            }
    }

public:
    struct Node {
        unsigned int Layer;
        std::array<unsigned int, Height> Indices;

        friend bool operator==(const Node &lhs, const Node &rhs) {
            return lhs.Layer == rhs.Layer && lhs.Indices == rhs.Indices;
        }

        friend std::ostream &operator<<(std::ostream &os, const Node &node) {
            os << "Node(Layer=" << node.Layer << ",Indices=[";
            for (unsigned int i = 0; i < Height; ++i)
                os << (i == 0 ? "" : ", ") << node.Indices[i];
            os << "])";
            return os;
        }

        struct Hash {
            size_t operator()(const Node &node) const {
                std::hash<unsigned int> hash;
                auto result = hash(node.Layer);
                for (auto index : node.Indices)
                    result = hash(result ^ index);
                return result;
            }
        };
    };

    struct Edge {
        Node Parent, Child;

        friend bool operator==(const Edge &lhs, const Edge &rhs) {
            return lhs.Parent == rhs.Parent && lhs.Child == rhs.Child;
        }

        friend std::ostream &operator<<(std::ostream &os, const Edge &edge) {
            os << "Edge(Parent=" << edge.Parent << ",Child=" << edge.Child << ")";
            return os;
        }

        struct Hash {
            size_t operator()(const Edge &edge) const {
                typename Node::Hash hash;
                return hash(edge.Parent) ^ hash(edge.Child);
            }
        };
    };

    std::array<unsigned int, Height> DownLinkCount, UpLinkCount;
    std::array<std::vector<Node>, Height + 1> NodesByLayer;
    std::unordered_map<Node, std::vector<Node>, typename Node::Hash> Parents, Children;

    FatTree(const std::array<unsigned int, Height> &downLinkCount, const std::array<unsigned int, Height> &upLinkCount)
        : DownLinkCount(downLinkCount), UpLinkCount(upLinkCount) {
        Initialize();
    }

    FatTree(unsigned int degree) {
        assert(degree >= 2 && degree % 2 == 0);
        DownLinkCount.fill(degree / 2);
        DownLinkCount[Height - 1] = degree;
        UpLinkCount.fill(degree / 2);
        UpLinkCount[0] = 1;
        Initialize();
    }

    std::vector<Node> GetClosestCommonAncestors(const std::vector<Node> &leaves) const {
        assert(leaves.size() > 0);
        unsigned int layer = 0;
        for (int i = Height - 1; i >= 0; --i) {
            bool same = true;
            for (auto leaf : leaves)
                if (leaf.Indices[i] != leaves.front().Indices[i]) {
                    same = false;
                    break;
                }
            if (!same) {
                layer = i + 1;
                break;
            }
        }
        std::array<unsigned int, Height> indices = {};
        for (unsigned int i = layer; i < Height; ++i)
            indices[i] = leaves.front().Indices[i];
        std::vector<Node> result;
        while (true) {
            result.push_back({layer, indices});
            unsigned int i = 0;
            for (; i < layer; ++i) {
                ++indices[i];
                if (indices[i] < UpLinkCount[i])
                    break;
                indices[i] = 0;
            }
            if (i == layer)
                break;
        }
        return result;
    }

    auto GetAggregationTree(const std::vector<Node> &leaves, Node root) const {
        assert(leaves.size() > 0);
        std::unordered_set<Node, typename Node::Hash> nodes(leaves.cbegin(), leaves.cend());
        std::vector<Edge> edges;
        std::queue<Node> queue;
        for (auto leaf : leaves)
            queue.push(leaf);
        while (!queue.empty()) {
            auto node = queue.front();
            queue.pop();
            if (node == root)
                continue;
            auto [layer, indices] = node;
            auto parentIndices = indices;
            parentIndices[layer] = root.Indices[layer];
            Node parent = {layer + 1, parentIndices};
            edges.push_back({parent, node});
            if (nodes.count(parent) == 0) {
                nodes.insert(parent);
                queue.push(parent);
            }
        }
        return std::make_pair(std::vector(nodes.cbegin(), nodes.cend()), std::move(edges));
    }
};

#pragma once

#include "fat_tree.hpp"
#include <iostream>
#include <optional>
#include <random>
#include <unordered_map>

template <unsigned int Height>
class FatTreeResource {
private:
    using Node = typename FatTree<Height>::Node;
    using Edge = typename FatTree<Height>::Edge;
    using AggrTree = typename FatTree<Height>::AggrTree;

    std::vector<unsigned int> m_NodeUsage;
    std::vector<unsigned int> m_EdgeUsage;

public:
    const FatTree<Height> *Topology;
    const std::optional<unsigned int> NodeQuota, LinkQuota;

    explicit FatTreeResource(const FatTree<Height> &topology, std::optional<unsigned int> nodeQuota,
                             std::optional<unsigned int> linkQuota)
        : m_NodeUsage(topology.Nodes.size(), 0), m_EdgeUsage(topology.Edges.size(), 0), Topology(&topology),
          NodeQuota(nodeQuota), LinkQuota(linkQuota) {
        assert(!NodeQuota || *NodeQuota > 0);
        assert(!LinkQuota || *LinkQuota > 0);
    }

    const std::vector<unsigned int> &GetNodeUsage() const { return m_NodeUsage; }
    const std::vector<unsigned int> &GetEdgeUsage() const { return m_EdgeUsage; }

    void Allocate(const AggrTree &tree) {
        const auto &[nodes, edges] = tree;
        for (auto node : nodes) {
            assert(!NodeQuota || m_NodeUsage[node->ID] < *NodeQuota)
            ++m_NodeUsage[node->ID];
        }
        for (auto edge : edges) {
            assert(!LinkQuota || m_EdgeUsage[edge->ID] < *LinkQuota)
            ++m_EdgeUsage[edge->ID];
        }
    }

    void Deallocate(const AggrTree &tree) {
        const auto &[nodes, edges] = tree;
        for (auto node : nodes) {
            assert(m_NodeUsage[node->ID] > 0);
            --m_NodeUsage[node->ID];
        }
        for (auto edge : edges) {
            assert(m_EdgeUsage[edge->ID] > 0);
            --m_EdgeUsage[edge->ID];
        }
    }

    bool CheckTreeConflict(const AggrTree &tree) const {
        const auto &[nodes, edges] = tree;
        if (NodeQuota)
            for (auto node : nodes)
                if (m_NodeUsage[node->ID] >= *NodeQuota)
                    return true;
        if (LinkQuota)
            for (auto edge : edges)
                if (m_EdgeUsage[edge->ID] >= *LinkQuota)
                    return true;
        return false;
    }
};

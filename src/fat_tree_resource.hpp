#pragma once

#include "fat_tree.hpp"
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
    std::unordered_map<unsigned int, AggrTree> m_RunningJobs;

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

    const std::vector<unsigned int> GetNodeUsage() const { return m_NodeUsage; }
    const std::vector<unsigned int> GetEdgeUsage() const { return m_EdgeUsage; }
    const std::unordered_map<unsigned int, AggrTree> GetRunningJobs() const { return m_RunningJobs; }

    void Allocate(unsigned int jobId, AggrTree &&tree) {
        assert(!CheckTreeConflict(tree));
        assert(m_RunningJobs.count(jobId) == 0);
        const auto &[nodes, edges] = tree;
        for (auto node : nodes)
            ++m_NodeUsage[node->ID];
        for (auto edge : edges)
            ++m_EdgeUsage[edge->ID];
        m_RunningJobs.try_emplace(jobId, std::move(tree));
    }

    void Deallocate(unsigned int jobId) {
        assert(m_RunningJobs.count(jobId) != 0);
        auto &[nodes, edges] = m_RunningJobs[jobId];
        for (auto node : nodes) {
            assert(m_NodeUsage[node->ID] > 0);
            --m_NodeUsage[node->ID];
        }
        for (auto edge : edges) {
            assert(m_EdgeUsage[edge->ID] > 0);
            --m_EdgeUsage[edge->ID];
        }
        m_RunningJobs.erase(jobId);
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

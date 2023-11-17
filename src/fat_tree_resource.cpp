#include "fat_tree_resource.hpp"
#include <cassert>

FatTreeResource::FatTreeResource(const FatTree &topology, std::optional<unsigned int> nodeQuota,
                                 std::optional<unsigned int> linkQuota)
    : m_NodeUsage(topology.Nodes.size(), 0), m_EdgeUsage(topology.Edges.size(), 0), Topology(&topology),
      NodeQuota(nodeQuota), LinkQuota(linkQuota) {
    assert(!NodeQuota || *NodeQuota > 0);
    assert(!LinkQuota || *LinkQuota > 0);
}

void FatTreeResource::Allocate(const AggrTree &tree) {
    const auto &[nodes, edges] = tree;
    for (auto node : nodes) {
        if (node->Layer == 0)
            continue;
        assert(!NodeQuota || m_NodeUsage[node->ID] < *NodeQuota);
        ++m_NodeUsage[node->ID];
    }
    for (auto edge : edges) {
        assert(!LinkQuota || m_EdgeUsage[edge->ID] < *LinkQuota);
        ++m_EdgeUsage[edge->ID];
    }
}

void FatTreeResource::Allocate(const std::vector<const Node *> &hosts) {
    for (auto node : hosts) {
        assert(node->Layer == 0);
        assert(!NodeQuota || m_NodeUsage[node->ID] < *NodeQuota);
        ++m_NodeUsage[node->ID];
    }
}

void FatTreeResource::Deallocate(const AggrTree &tree) {
    const auto &[nodes, edges] = tree;
    for (auto node : nodes) {
        if (node->Layer == 0)
            continue;
        assert(m_NodeUsage[node->ID] > 0);
        --m_NodeUsage[node->ID];
    }
    for (auto edge : edges) {
        assert(m_EdgeUsage[edge->ID] > 0);
        --m_EdgeUsage[edge->ID];
    }
}

void FatTreeResource::Deallocate(const std::vector<const Node *> &hosts) {
    for (auto node : hosts) {
        assert(node->Layer == 0);
        assert(m_NodeUsage[node->ID] > 0);
        --m_NodeUsage[node->ID];
    }
}

bool FatTreeResource::CheckTreeConflict(const AggrTree &tree) const {
    const auto &[nodes, edges] = tree;
    if (NodeQuota)
        for (auto node : nodes) {
            if (node->Layer == 0)
                continue;
            if (m_NodeUsage[node->ID] >= *NodeQuota)
                return true;
        }
    if (LinkQuota)
        for (auto edge : edges)
            if (m_EdgeUsage[edge->ID] >= *LinkQuota)
                return true;
    return false;
}

bool FatTreeResource::CheckTreeConflict(const AggrTree &tree1, const AggrTree &tree2) {
    // TODO: Optimize
    const auto &[nodes1, edges1] = tree1;
    const auto &[nodes2, edges2] = tree2;
    for (auto node1 : nodes1)
        for (auto node2 : nodes2)
            if (node1 == node2)
                return true;
    for (auto edge1 : edges1)
        for (auto edge2 : edges2)
            if (edge1 == edge2)
                return true;
    return false;
}

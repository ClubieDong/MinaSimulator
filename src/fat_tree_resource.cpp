#include "fat_tree_resource.hpp"
#include <cassert>

unsigned int FatTreeResource::CalcHostFragments(bool available, unsigned int beginHostIdx, unsigned int hostCountInPod,
                                                unsigned int level) const {
    const auto &hosts = Topology->NodesByLayer[0];
    bool allAvailable = true, noneAvailable = true;
    for (auto hostIdx = beginHostIdx; hostIdx < beginHostIdx + hostCountInPod; ++hostIdx)
        if (m_NodeUsage[hosts[hostIdx]->ID] == 0)
            noneAvailable = false;
        else
            allAvailable = false;
    if (allAvailable)
        return available;
    if (noneAvailable)
        return !available;
    assert(level > 0);
    assert(hostCountInPod > 1);
    unsigned int sum = 0, hostCountInSubPod = hostCountInPod / Topology->DownLinkCount[level - 1];
    for (unsigned int subPodBeginHostIdx = beginHostIdx; subPodBeginHostIdx < beginHostIdx + hostCountInPod;
         subPodBeginHostIdx += hostCountInSubPod)
        sum += CalcHostFragments(available, subPodBeginHostIdx, hostCountInSubPod, level - 1);
    return sum;
}

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

bool FatTreeResource::CheckTreeConflict(const AggrTree &tree1, const AggrTree &tree2) const {
    // TODO: Optimize
    const auto &[nodes1, edges1] = tree1;
    const auto &[nodes2, edges2] = tree2;
    if (NodeQuota && *NodeQuota < 2)
        for (auto node1 : nodes1)
            for (auto node2 : nodes2)
                if (node1 == node2)
                    return true;
    if (LinkQuota && *LinkQuota < 2)
        for (auto edge1 : edges1)
            for (auto edge2 : edges2)
                if (edge1 == edge2)
                    return true;
    return false;
}

unsigned int FatTreeResource::CalcHostFragments(bool available) const {
    return CalcHostFragments(available, 0, Topology->NodesByLayer[0].size(), Topology->Height);
}

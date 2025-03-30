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
    for (auto node : tree.Nodes) {
        if (node->Layer == 0)
            continue;
        assert(!NodeQuota || m_NodeUsage[node->ID] < *NodeQuota);
        ++m_NodeUsage[node->ID];
    }
    for (auto edge : tree.Edges) {
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
    for (auto node : tree.Nodes) {
        if (node->Layer == 0)
            continue;
        assert(m_NodeUsage[node->ID] > 0);
        --m_NodeUsage[node->ID];
    }
    for (auto edge : tree.Edges) {
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
    if (NodeQuota)
        for (auto node : tree.Nodes) {
            if (node->Layer == 0)
                continue;
            if (m_NodeUsage[node->ID] >= *NodeQuota)
                return true;
        }
    if (LinkQuota)
        for (auto edge : tree.Edges)
            if (m_EdgeUsage[edge->ID] >= *LinkQuota)
                return true;
    return false;
}

bool FatTreeResource::CheckTreeConflict(const AggrTree &tree1, const AggrTree &tree2) const {
    if (NodeQuota && *NodeQuota < 2)
        for (auto node : tree1.Nodes)
            if (tree2.NodeSet[node->ID])
                return true;
    if (LinkQuota && *LinkQuota < 2)
        for (auto edge : tree1.Edges)
            if (tree2.EdgeSet[edge->ID])
                return true;
    return false;
}

unsigned int FatTreeResource::CalcHostFragments(bool available) const {
    return CalcHostFragments(available, 0, Topology->NodesByLayer[0].size(), Topology->Height);
}

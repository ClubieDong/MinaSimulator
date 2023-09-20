#pragma once

#include "fat_tree.hpp"
#include <optional>
#include <random>

template <unsigned int Height>
class JobAllocator {
private:
    using Node = typename FatTree<Height>::Node;
    using Edge = typename FatTree<Height>::Edge;
    using AggrTree = typename FatTree<Height>::AggrTree;

    std::vector<unsigned int> m_NodeUsage;
    std::vector<unsigned int> m_EdgeUsage;
    std::unordered_map<unsigned int, AggrTree> m_RunningJobs;

public:
    enum class AllocRes {
        Sharp,
        NonSharp,
        Fail,
    };

    using AllocHostMethod = std::optional<std::vector<const Node *>> (JobAllocator::*)(unsigned int) const;
    using AllocTreeMethod = std::optional<AggrTree> (JobAllocator::*)(const std::vector<const Node *> &) const;

    const FatTree<Height> *Topology;
    const std::optional<unsigned int> NodeQuota, LinkQuota;

    explicit JobAllocator(const FatTree<Height> &topology, std::optional<unsigned int> nodeQuota,
                          std::optional<unsigned int> linkQuota)
        : m_NodeUsage(topology.Nodes.size(), 0), m_EdgeUsage(topology.Edges.size(), 0), Topology(&topology),
          NodeQuota(nodeQuota), LinkQuota(linkQuota) {
        assert(!NodeQuota || *NodeQuota > 0);
        assert(!LinkQuota || *LinkQuota > 0);
    }

    void DoAllocate(unsigned int jobId, AggrTree &&tree) {
        assert(!CheckTreeConflict(tree));
        assert(m_RunningJobs.count(jobId) == 0);
        const auto &[nodes, edges] = tree;
        for (auto node : nodes)
            ++m_NodeUsage[node->ID];
        for (auto edge : edges)
            ++m_EdgeUsage[edge->ID];
        m_RunningJobs.try_emplace(jobId, std::move(tree));
    }

    void DoDeallocate(unsigned int jobId) {
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

    std::pair<AllocRes, std::optional<AggrTree>> TryAllocateHostsAndTree(unsigned int nHosts,
                                                                         AllocHostMethod allocHostMethod,
                                                                         AllocTreeMethod allocTreeMethod) const {
        assert(nHosts > 0);
        auto chosenHosts = (this->*allocHostMethod)(nHosts);
        if (!chosenHosts)
            return {AllocRes::Fail, std::nullopt};
        auto tree = (this->*allocTreeMethod)(*chosenHosts);
        if (!tree) {
            std::optional<AggrTree> tree(std::in_place, std::move(*chosenHosts), std::vector<const Edge *>{});
            return {AllocRes::NonSharp, std::move(tree)};
        }
        return {AllocRes::Sharp, std::move(tree)};
    }

    std::optional<std::vector<const Node *>> TryAllocateRandomHosts(unsigned int nHosts) const {
        assert(nHosts > 0);
        std::vector<const Node *> availableHosts;
        for (unsigned int hostId = 0; hostId < Topology->NodesByLayer[0].size(); ++hostId)
            if (m_NodeUsage[hostId] == 0)
                availableHosts.push_back(Topology->NodesByLayer[0][hostId]);
        if (availableHosts.size() < nHosts)
            return std::nullopt;
        std::vector<const Node *> chosenHosts;
        thread_local std::default_random_engine engine(std::random_device{}());
        std::sample(availableHosts.cbegin(), availableHosts.cend(), std::back_inserter(chosenHosts), nHosts, engine);
        return chosenHosts;
    }

    std::optional<std::vector<const Node *>> TryAllocateFirstHosts(unsigned int nHosts) const {
        assert(nHosts > 0);
        std::vector<const Node *> availableHosts;
        for (unsigned int hostId = 0; hostId < Topology->NodesByLayer[0].size(); ++hostId)
            if (m_NodeUsage[hostId] == 0) {
                availableHosts.push_back(Topology->NodesByLayer[0][hostId]);
                if (availableHosts.size() == nHosts)
                    return availableHosts;
            }
        return std::nullopt;
    }

    std::optional<AggrTree> TryAllocateRandomTree(const std::vector<const Node *> &chosenHosts) const {
        std::vector<AggrTree> trees;
        for (auto root : Topology->GetClosestCommonAncestors(chosenHosts)) {
            auto tree = Topology->GetAggregationTree(chosenHosts, root);
            if (!CheckTreeConflict(tree))
                trees.push_back(std::move(tree));
        }
        if (trees.empty())
            return std::nullopt;
        thread_local std::default_random_engine engine(std::random_device{}());
        std::uniform_int_distribution<std::size_t> random(0, trees.size() - 1);
        return trees[random(engine)];
    }

    std::optional<AggrTree> TryAllocateFirstTree(const std::vector<const Node *> &chosenHosts) const {
        for (auto root : Topology->GetClosestCommonAncestors(chosenHosts)) {
            auto tree = Topology->GetAggregationTree(chosenHosts, root);
            if (!CheckTreeConflict(tree))
                return tree;
        }
        return std::nullopt;
    }

    inline static std::pair<const char *, AllocHostMethod> AllocHostMethods[] = {
        {"RandomHosts", &JobAllocator::TryAllocateRandomHosts},
        {"FirstHosts", &JobAllocator::TryAllocateFirstHosts},
    };

    inline static std::pair<const char *, AllocTreeMethod> AllocTreeMethods[] = {
        {"RandomTree", &JobAllocator::TryAllocateRandomTree},
        {"FirstTree", &JobAllocator::TryAllocateFirstTree},
    };
};

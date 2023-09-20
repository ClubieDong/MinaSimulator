#pragma once

#include "fat_tree.hpp"
#include <optional>
#include <random>

template <unsigned int Height>
class JobAllocator {
private:
    using Node = typename FatTree<Height>::Node;
    using Edge = typename FatTree<Height>::Edge;

    std::vector<unsigned int> m_NodeUsage;
    std::vector<unsigned int> m_EdgeUsage;
    unsigned int m_NextJobId = 0;
    std::unordered_map<unsigned int, std::pair<std::vector<const Node *>, std::vector<const Edge *>>> m_RunningJobs;

public:
    enum class AllocRes {
        Sharp,
        NonSharp,
        Fail,
    };

    using AllocMethod = std::optional<std::vector<const Node *>> (JobAllocator::*)(unsigned int);

    const FatTree<Height> *Topology;
    std::optional<unsigned int> NodeQuota, LinkQuota;

    explicit JobAllocator(const FatTree<Height> &topology, std::optional<unsigned int> nodeQuota,
                          std::optional<unsigned int> linkQuota)
        : m_NodeUsage(topology.Nodes.size(), 0), m_EdgeUsage(topology.Edges.size(), 0), Topology(&topology),
          NodeQuota(nodeQuota), LinkQuota(linkQuota) {
        assert(!NodeQuota || *NodeQuota > 0);
        assert(!LinkQuota || *LinkQuota > 0);
    }

    std::pair<AllocRes, unsigned int> Allocate(unsigned int nHosts, AllocMethod method) {
        assert(nHosts > 0);
        auto chosenHosts = (this->*method)(nHosts);
        if (!chosenHosts)
            return {AllocRes::Fail, -1};
        std::vector<std::tuple<const Node *, std::vector<const Node *>, std::vector<const Edge *>>> trees;
        for (auto root : Topology->GetClosestCommonAncestors(*chosenHosts)) {
            auto [nodes, edges] = Topology->GetAggregationTree(*chosenHosts, root);
            if (NodeQuota && std::any_of(nodes.cbegin(), nodes.cend(),
                                         [this](const Node *node) { return m_NodeUsage[node->ID] + 1 > *NodeQuota; }))
                continue;
            if (LinkQuota && std::any_of(edges.cbegin(), edges.cend(),
                                         [this](const Edge *edge) { return m_EdgeUsage[edge->ID] + 1 > *LinkQuota; }))
                continue;
            trees.emplace_back(root, std::move(nodes), std::move(edges));
        }
        std::vector<const Node *> nodes;
        std::vector<const Edge *> edges;
        AllocRes allocRes;
        if (trees.empty()) {
            nodes = std::move(*chosenHosts);
            allocRes = AllocRes::NonSharp;
        } else {
            thread_local std::default_random_engine engine(std::random_device{}());
            std::uniform_int_distribution<std::size_t> random(0, trees.size() - 1);
            auto chosenTreeIdx = random(engine);
            nodes = std::move(std::get<1>(trees[chosenTreeIdx]));
            edges = std::move(std::get<2>(trees[chosenTreeIdx]));
            allocRes = AllocRes::Sharp;
        }
        for (auto node : nodes)
            ++m_NodeUsage[node->ID];
        for (auto edge : edges)
            ++m_EdgeUsage[edge->ID];
        auto jobId = m_NextJobId++;
        m_RunningJobs.try_emplace(jobId, std::make_pair(std::move(nodes), std::move(edges)));
        return {allocRes, jobId};
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

    std::optional<std::vector<const Node *>> AllocateRandom(unsigned int nHosts) {
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

    std::optional<std::vector<const Node *>> AllocateFirst(unsigned int nHosts) {
        std::vector<const Node *> availableHosts;
        for (unsigned int hostId = 0; hostId < Topology->NodesByLayer[0].size(); ++hostId)
            if (m_NodeUsage[hostId] == 0) {
                availableHosts.push_back(Topology->NodesByLayer[0][hostId]);
                if (availableHosts.size() == nHosts)
                    return availableHosts;
            }
        return std::nullopt;
    }

    inline static std::pair<const char *, AllocMethod> AllocMethods[] = {
        {"AllocateRandom", &JobAllocator::AllocateRandom},
        {"AllocateFirst", &JobAllocator::AllocateFirst},
    };
};

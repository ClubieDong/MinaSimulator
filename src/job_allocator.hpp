#pragma once

#include "fat_tree.hpp"
#include <algorithm>
#include <optional>
#include <random>

template <unsigned int Height>
class JobAllocator {
private:
    using Node = typename FatTree<Height>::Node;
    using Edge = typename FatTree<Height>::Edge;

    std::unordered_map<Node, unsigned int, typename Node::Hash> m_NodeUsage;
    std::unordered_map<Edge, unsigned int, typename Edge::Hash> m_EdgeUsage;
    unsigned int m_NextJobId = 0;
    std::unordered_map<unsigned int, std::pair<std::vector<Node>, std::vector<Edge>>> m_RunningJobs;

public:
    enum class AllocRes {
        Sharp,
        NonSharp,
        Fail,
    };

    using AllocMethod = std::optional<std::vector<Node>> (JobAllocator::*)(unsigned int);

    FatTree<Height> Topology;
    std::optional<unsigned int> NodeQuota, LinkQuota;

    JobAllocator(FatTree<Height> &&topology, std::optional<unsigned int> nodeQuota,
                 std::optional<unsigned int> linkQuota)
        : Topology(std::move(topology)), NodeQuota(nodeQuota), LinkQuota(linkQuota) {
        assert(!NodeQuota || *NodeQuota > 0);
        assert(!LinkQuota || *LinkQuota > 0);
    }

    std::pair<AllocRes, unsigned int> Allocate(unsigned int nHosts, AllocMethod method) {
        assert(nHosts > 0);
        auto chosenHosts = (this->*method)(nHosts);
        if (!chosenHosts)
            return {AllocRes::Fail, -1};
        std::unordered_map<Node, std::pair<std::vector<Node>, std::vector<Edge>>, typename Node::Hash> roots;
        for (auto root : Topology.GetClosestCommonAncestors(*chosenHosts)) {
            auto [nodes, edges] = Topology.GetAggregationTree(*chosenHosts, root);
            if (NodeQuota && std::any_of(nodes.begin(), nodes.end(),
                                         [this](Node node) { return m_NodeUsage[node] + 1 > *NodeQuota; }))
                continue;
            if (LinkQuota && std::any_of(edges.begin(), edges.end(),
                                         [this](Edge edge) { return m_EdgeUsage[edge] + 1 > *LinkQuota; }))
                continue;
            roots[root] = std::make_pair(std::move(nodes), std::move(edges));
        }
        std::vector<Node> nodes;
        std::vector<Edge> edges;
        AllocRes allocRes;
        if (roots.empty()) {
            nodes = std::move(*chosenHosts);
            allocRes = AllocRes::NonSharp;
        } else {
            thread_local std::default_random_engine engine(std::random_device{}());
            std::uniform_int_distribution<std::size_t> random(0, roots.size() - 1);
            std::tie(nodes, edges) = std::move(std::next(roots.cbegin(), random(engine))->second);
            allocRes = AllocRes::Sharp;
        }
        for (auto node : nodes)
            ++m_NodeUsage[node];
        for (auto edge : edges)
            ++m_EdgeUsage[edge];
        auto jobId = m_NextJobId++;
        m_RunningJobs[jobId] = std::make_pair(std::move(nodes), std::move(edges));
        return {allocRes, jobId};
    }

    void Deallocate(unsigned int jobId) {
        auto &[nodes, edges] = m_RunningJobs[jobId];
        for (auto node : nodes) {
            assert(m_NodeUsage[node] > 0);
            --m_NodeUsage[node];
        }
        for (auto edge : edges) {
            assert(m_EdgeUsage[edge] > 0);
            --m_EdgeUsage[edge];
        }
        m_RunningJobs.erase(jobId);
    }

    std::optional<std::vector<Node>> AllocateRandom(unsigned int nHosts) {
        std::vector<Node> availableHosts;
        for (auto node : Topology.NodesByLayer[0])
            if (m_NodeUsage[node] == 0)
                availableHosts.push_back(node);
        if (availableHosts.size() < nHosts)
            return std::nullopt;
        std::vector<Node> result;
        thread_local std::default_random_engine engine(std::random_device{}());
        std::sample(availableHosts.cbegin(), availableHosts.cend(), std::back_inserter(result), nHosts, engine);
        return result;
    }

    std::optional<std::vector<Node>> AllocateFirst(unsigned int nHosts) {
        std::vector<Node> availableHosts;
        for (auto node : Topology.NodesByLayer[0])
            if (m_NodeUsage[node] == 0) {
                availableHosts.push_back(node);
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

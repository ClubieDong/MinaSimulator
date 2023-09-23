#pragma once

#include "fat_tree_resource.hpp"

template <unsigned int Height>
class RandomHostsAllocator {
private:
    using Node = typename FatTree<Height>::Node;

public:
    const FatTree<Height> *const Topology;
    const FatTreeResource<Height> *const Resources;

    explicit RandomHostsAllocator(const FatTreeResource<Height> &resources)
        : Topology(resources.Topology), Resources(&resources) {}

    std::optional<std::vector<const Node *>> operator()(unsigned int nHosts) {
        assert(nHosts > 0);
        const auto &nodeUsage = Resources->GetNodeUsage();
        const auto &hosts = Topology->NodesByLayer[0];
        std::vector<const Node *> availableHosts;
        for (unsigned int hostId = 0; hostId < hosts.size(); ++hostId)
            if (nodeUsage[hostId] == 0)
                availableHosts.push_back(hosts[hostId]);
        if (availableHosts.size() < nHosts)
            return std::nullopt;
        std::vector<const Node *> chosenHosts;
        thread_local std::default_random_engine engine(std::random_device{}());
        std::sample(availableHosts.cbegin(), availableHosts.cend(), std::back_inserter(chosenHosts), nHosts, engine);
        return chosenHosts;
    }
};

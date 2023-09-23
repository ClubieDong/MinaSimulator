#pragma once

#include "fat_tree_resource.hpp"

template <unsigned int Height>
class FirstHostsAllocator {
private:
    using Node = typename FatTree<Height>::Node;

public:
    const FatTree<Height> *const Topology;
    const FatTreeResource<Height> *const Resources;

    explicit FirstHostsAllocator(const FatTreeResource<Height> &resources)
        : Topology(resources.Topology), Resources(&resources) {}

    std::optional<std::vector<const Node *>> operator()(unsigned int nHosts) {
        assert(nHosts > 0);
        const auto &nodeUsage = Resources->GetNodeUsage();
        const auto &hosts = Topology->NodesByLayer[0];
        std::vector<const Node *> availableHosts;
        for (unsigned int hostId = 0; hostId < hosts.size(); ++hostId)
            if (nodeUsage[hostId] == 0) {
                availableHosts.push_back(hosts[hostId]);
                if (availableHosts.size() == nHosts)
                    return availableHosts;
            }
        return std::nullopt;
    }
};

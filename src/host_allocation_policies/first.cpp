#include "first.hpp"
#include <cassert>

std::optional<std::vector<const FatTree::Node *>>
FirstHostAllocationPolicy::operator()(const FatTreeResource &resources, unsigned int hostCount) const {
    assert(hostCount > 0);
    const auto &nodeUsage = resources.GetNodeUsage();
    const auto &hosts = resources.Topology->NodesByLayer[0];
    std::vector<const Node *> availableHosts;
    for (unsigned int hostId = 0; hostId < hosts.size(); ++hostId)
        if (nodeUsage[hostId] == 0) {
            availableHosts.push_back(hosts[hostId]);
            if (availableHosts.size() == hostCount)
                return availableHosts;
        }
    return std::nullopt;
}

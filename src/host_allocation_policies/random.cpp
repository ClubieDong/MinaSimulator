#include "random.hpp"
#include <algorithm>
#include <cassert>
#include <random>

std::optional<std::vector<const FatTree::Node *>>
RandomHostAllocationPolicy::operator()(const FatTreeResource &resources, unsigned int hostCount) const {
    assert(hostCount > 0);
    const auto &nodeUsage = resources.GetNodeUsage();
    const auto &hosts = resources.Topology->NodesByLayer[0];
    std::vector<const Node *> availableHosts;
    for (unsigned int hostId = 0; hostId < hosts.size(); ++hostId)
        if (nodeUsage[hostId] == 0)
            availableHosts.push_back(hosts[hostId]);
    if (availableHosts.size() < hostCount)
        return std::nullopt;
    std::vector<const Node *> chosenHosts;
    thread_local std::default_random_engine engine(42);
    std::sample(availableHosts.cbegin(), availableHosts.cend(), std::back_inserter(chosenHosts), hostCount, engine);
    return chosenHosts;
}

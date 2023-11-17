#include "smart.hpp"
#include <algorithm>
#include <cassert>
#include <limits>

unsigned int SmartHostAllocationPolicy::TryAllocate(const FatTreeResource &resources, unsigned int beginHostIdx,
                                                    unsigned int hostCountInPod, unsigned int level,
                                                    std::vector<TryAllocateResult> &result) const {
    assert(result.size() >= 2);
    const auto &nodeUsage = resources.GetNodeUsage();
    const auto &hosts = resources.Topology->NodesByLayer[0];
    if (level == 0) {
        assert(hostCountInPod == 1);
        if (nodeUsage[hosts[beginHostIdx]->ID] > 0) {
            result[0].FragmentScore = 0;
            result[0].Hosts.clear();
            return 0;
        }
        result[0].FragmentScore = Alpha;
        result[0].Hosts.clear();
        result[1].FragmentScore = 1;
        result[1].Hosts.clear();
        result[1].Hosts.push_back(hosts[beginHostIdx]);
        return 1;
    }
    result[0].FragmentScore = 0.0;
    result[0].Hosts.clear();
    unsigned int totalAvailHostCount = 0,
                 hostCountInSubPod = hostCountInPod / resources.Topology->DownLinkCount[level - 1],
                 requiredHostCount = result.size() - 1;
    std::vector<TryAllocateResult> subPodResult(requiredHostCount + 1);
    for (unsigned int subPodBeginHostIdx = beginHostIdx; subPodBeginHostIdx < beginHostIdx + hostCountInPod;
         subPodBeginHostIdx += hostCountInSubPod) {
        auto subPodAvailHostCount =
            TryAllocate(resources, subPodBeginHostIdx, hostCountInSubPod, level - 1, subPodResult);
        for (int hostCount = std::min(totalAvailHostCount + subPodAvailHostCount, requiredHostCount); hostCount >= 0;
             --hostCount) {
            double minFragmentScore = std::numeric_limits<double>::max();
            unsigned int minI;
            for (unsigned int i = std::max(0, hostCount - static_cast<int>(totalAvailHostCount)), j = hostCount - i;
                 i <= std::min<unsigned int>(subPodAvailHostCount, hostCount); ++i, --j)
                if (minFragmentScore > subPodResult[i].FragmentScore + result[j].FragmentScore) {
                    minFragmentScore = subPodResult[i].FragmentScore + result[j].FragmentScore;
                    minI = i;
                }
            result[hostCount].FragmentScore = minFragmentScore;
            result[hostCount].Hosts.resize(hostCount);
            for (unsigned int i = 0; i < hostCount - minI; ++i)
                result[hostCount].Hosts[i] = result[hostCount - minI].Hosts[i];
            for (unsigned int i = hostCount - minI; i < static_cast<unsigned int>(hostCount); ++i)
                result[hostCount].Hosts[i] = subPodResult[minI].Hosts[i - (hostCount - minI)];
        }
        totalAvailHostCount += subPodAvailHostCount;
    }
    if (totalAvailHostCount == hostCountInPod) {
        result[0].FragmentScore = Alpha;
        if (hostCountInPod <= requiredHostCount)
            result[hostCountInPod].FragmentScore = 1;
    }
    return totalAvailHostCount;
}

std::optional<std::vector<const FatTree::Node *>>
SmartHostAllocationPolicy::operator()(const FatTreeResource &resources, unsigned int hostCount) const {
    assert(hostCount > 0);
    std::vector<TryAllocateResult> result(hostCount + 1);
    auto nAvailHosts = TryAllocate(resources, 0, resources.Topology->NodesByLayer[0].size(), FatTree::Height, result);
    if (nAvailHosts < hostCount)
        return std::nullopt;
    assert(result[hostCount].Hosts.size() == hostCount);
    return result[hostCount].Hosts;
}

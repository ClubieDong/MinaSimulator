#pragma once

#include "fat_tree_resource.hpp"
#include <iostream>

template <unsigned int Height>
class SmartHostsAllocator {
private:
    using Node = typename FatTree<Height>::Node;

    struct TryAllocateResult {
        double FragmentScore;
        std::vector<const Node *> Hosts;
    };

    unsigned int TryAllocate(unsigned int beginHostIdx, unsigned int nHostsInPod, unsigned int level,
                             std::vector<TryAllocateResult> &result) const {
        assert(result.size() >= 2);
        const auto &nodeUsage = Resources->GetNodeUsage();
        const auto &hosts = Topology->NodesByLayer[0];
        if (level == 0) {
            assert(nHostsInPod == 1);
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
        unsigned int nTotalAvailHosts = 0, nHostsInSubPod = nHostsInPod / Topology->DownLinkCount[level - 1],
                     nRequiredHosts = result.size() - 1;
        std::vector<TryAllocateResult> subPodResult(nRequiredHosts + 1);
        for (unsigned int subPodBeginHostIdx = beginHostIdx; subPodBeginHostIdx < beginHostIdx + nHostsInPod;
             subPodBeginHostIdx += nHostsInSubPod) {
            auto nSubPodAvailHosts = TryAllocate(subPodBeginHostIdx, nHostsInSubPod, level - 1, subPodResult);
            for (int nHosts = std::min(nTotalAvailHosts + nSubPodAvailHosts, nRequiredHosts); nHosts >= 0; --nHosts) {
                double minFragmentScore = std::numeric_limits<double>::max();
                unsigned int minI;
                for (unsigned int i = std::max(0, nHosts - static_cast<int>(nTotalAvailHosts)), j = nHosts - i;
                     i <= std::min<unsigned int>(nSubPodAvailHosts, nHosts); ++i, --j)
                    if (minFragmentScore > subPodResult[i].FragmentScore + result[j].FragmentScore) {
                        minFragmentScore = subPodResult[i].FragmentScore + result[j].FragmentScore;
                        minI = i;
                    }
                result[nHosts].FragmentScore = minFragmentScore;
                result[nHosts].Hosts.resize(nHosts);
                for (unsigned int i = 0; i < nHosts - minI; ++i)
                    result[nHosts].Hosts[i] = result[nHosts - minI].Hosts[i];
                for (unsigned int i = nHosts - minI; i < static_cast<unsigned int>(nHosts); ++i)
                    result[nHosts].Hosts[i] = subPodResult[minI].Hosts[i - (nHosts - minI)];
            }
            nTotalAvailHosts += nSubPodAvailHosts;
        }
        if (nTotalAvailHosts == nHostsInPod) {
            result[0].FragmentScore = Alpha;
            if (nHostsInPod <= nRequiredHosts)
                result[nHostsInPod].FragmentScore = 1;
        }
        return nTotalAvailHosts;
    }

public:
    const FatTree<Height> *const Topology;
    const FatTreeResource<Height> *const Resources;
    const double Alpha;

    explicit SmartHostsAllocator(const FatTreeResource<Height> &resources, double alpha)
        : Topology(resources.Topology), Resources(&resources), Alpha(alpha) {}

    std::optional<std::vector<const Node *>> operator()(unsigned int nRequiredHosts) const {
        assert(nRequiredHosts > 0);
        std::vector<TryAllocateResult> result(nRequiredHosts + 1);
        auto nAvailHosts = TryAllocate(0, Topology->NodesByLayer[0].size(), Height, result);
        if (nAvailHosts < nRequiredHosts)
            return std::nullopt;
        assert(result[nRequiredHosts].Hosts.size() == nRequiredHosts);
        return result[nRequiredHosts].Hosts;
    }
};

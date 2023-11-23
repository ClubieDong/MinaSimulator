#pragma once

#include "fat_tree.hpp"
#include "fat_tree_resource.hpp"
#include <optional>
#include <vector>

class SmartHostAllocationPolicy {
private:
    using Node = typename FatTree::Node;

    struct TryAllocateResult {
        double FragmentScore;
        std::vector<const Node *> Hosts;
    };

    unsigned int TryAllocate(const FatTreeResource &resources, unsigned int beginHostIdx, unsigned int hostCountInPod,
                             unsigned int level, std::vector<TryAllocateResult> &result) const;

public:
    const double Alpha;

    explicit SmartHostAllocationPolicy(double alpha = 0.5) : Alpha(alpha) {}

    std::optional<std::vector<const FatTree::Node *>> operator()(const FatTreeResource &resources,
                                                                 unsigned int hostCount) const;
};

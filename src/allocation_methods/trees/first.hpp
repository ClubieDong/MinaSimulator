#pragma once

#include "fat_tree_resource.hpp"

template <unsigned int Height>
class FirstTreeAllocator {
private:
    using Node = typename FatTree<Height>::Node;
    using AggrTree = typename FatTree<Height>::AggrTree;

public:
    const FatTree<Height> *const Topology;
    const FatTreeResource<Height> *const Resources;

    explicit FirstTreeAllocator(const FatTreeResource<Height> &resources)
        : Topology(resources.Topology), Resources(&resources) {}

    std::optional<AggrTree> operator()(const std::vector<const Node *> &chosenHosts) {
        for (auto root : Topology->GetClosestCommonAncestors(chosenHosts)) {
            auto tree = Topology->GetAggregationTree(chosenHosts, root);
            if (!Resources->CheckTreeConflict(tree))
                return tree;
        }
        return std::nullopt;
    }
};
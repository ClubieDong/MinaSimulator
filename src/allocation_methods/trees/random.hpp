#pragma once

#include "fat_tree_resource.hpp"

template <unsigned int Height>
class RandomTreeAllocator {
private:
    using Node = typename FatTree<Height>::Node;
    using AggrTree = typename FatTree<Height>::AggrTree;

public:
    const FatTree<Height> *const Topology;
    const FatTreeResource<Height> *const Resources;

    explicit RandomTreeAllocator(const FatTreeResource<Height> &resources)
        : Topology(resources.Topology), Resources(&resources) {}

    std::optional<AggrTree> operator()(const std::vector<const Node *> &chosenHosts) {
        std::vector<AggrTree> trees;
        for (auto root : Topology->GetClosestCommonAncestors(chosenHosts)) {
            auto tree = Topology->GetAggregationTree(chosenHosts, root);
            if (!Resources->CheckTreeConflict(tree))
                trees.push_back(std::move(tree));
        }
        if (trees.empty())
            return std::nullopt;
        thread_local std::default_random_engine engine(std::random_device{}());
        std::uniform_int_distribution<std::size_t> random(0, trees.size() - 1);
        return trees[random(engine)];
    }
};

#pragma once

#include "fat_tree_resource.hpp"
#include "utils.hpp"

template <unsigned int Height, typename HostAllocator, typename TreeAllocator>
class HostTreeAllocator {
private:
    using Node = typename FatTree<Height>::Node;
    using Edge = typename FatTree<Height>::Edge;
    using AggrTree = typename FatTree<Height>::AggrTree;

    HostAllocator m_HostAllocator;
    TreeAllocator m_TreeAllocator;

public:
    const FatTree<Height> *const Topology;
    const FatTreeResource<Height> *const Resources;

    explicit HostTreeAllocator(const FatTreeResource<Height> &resources, HostAllocator &&hostAllocator,
                               TreeAllocator &&treeAllocator)
        : m_HostAllocator(std::move(hostAllocator)), m_TreeAllocator(std::move(treeAllocator)),
          Topology(resources.Topology), Resources(&resources) {}

    std::pair<AllocRes, std::optional<AggrTree>> operator()(unsigned int nHosts) {
        assert(nHosts > 0);
        auto chosenHosts = m_HostAllocator(nHosts);
        if (!chosenHosts)
            return {AllocRes::Fail, std::nullopt};
        auto tree = m_TreeAllocator(*chosenHosts);
        if (tree)
            return {AllocRes::Sharp, std::move(tree)};
        tree.emplace(std::move(*chosenHosts), std::vector<const Edge *>());
        return {AllocRes::NonSharp, std::move(tree)};
    }
};

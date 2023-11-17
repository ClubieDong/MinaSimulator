#pragma once

#include "fat_tree.hpp"
#include "fat_tree_resource.hpp"
#include <optional>
#include <vector>

class RandomHostAllocationPolicy {
private:
    using Node = typename FatTree::Node;

public:
    std::optional<std::vector<const Node *>> operator()(const FatTreeResource &resources, unsigned int hostCount) const;
};

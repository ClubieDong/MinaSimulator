#pragma once

#include "fat_tree.hpp"
#include "fat_tree_resource.hpp"
#include <optional>
#include <vector>

class FirstHostAllocationPolicy {
public:
    std::optional<std::vector<const FatTree::Node *>> operator()(const FatTreeResource &resources,
                                                                 unsigned int hostCount) const;
};

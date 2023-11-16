#pragma once

#include "fat_tree_resource.hpp"
#include "job.hpp"
#include <memory>
#include <vector>

class NoneTreeBuildingPolicy {
public:
    void operator()(const FatTreeResource &, const std::vector<std::unique_ptr<Job>> &,
                    const std::vector<Job *> &) const {}
};

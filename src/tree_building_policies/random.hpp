#pragma once

#include "fat_tree_resource.hpp"
#include "job.hpp"
#include <memory>
#include <vector>

class RandomTreeBuildingPolicy {
private:
    using AggrTree = typename FatTree::AggrTree;

public:
    void operator()(const FatTreeResource &resources, const std::vector<std::unique_ptr<Job>> &jobs,
                    const std::vector<Job *> &newJobs) const;
};

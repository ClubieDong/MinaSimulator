#pragma once

#include "fat_tree_resource.hpp"
#include "job.hpp"
#include <memory>
#include <vector>

class SmartTreeBuildingPolicy {
public:
    const std::optional<unsigned int> MaxTreeCount;

    explicit SmartTreeBuildingPolicy(std::optional<unsigned int> maxTreeCount) : MaxTreeCount(maxTreeCount) {}

    void operator()(const FatTreeResource &resources, const std::vector<std::unique_ptr<Job>> &jobs,
                    const std::vector<Job *> &newJobs) const;
};

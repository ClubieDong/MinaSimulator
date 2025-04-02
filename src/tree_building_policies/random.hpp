#pragma once

#include "fat_tree_resource.hpp"
#include "job.hpp"
#include <memory>
#include <vector>

class RandomTreeBuildingPolicy {
private:
    bool m_CheckConflict;

public:
    explicit RandomTreeBuildingPolicy(bool checkConflict) : m_CheckConflict(checkConflict) {}

    void operator()(const FatTreeResource &resources, const std::vector<std::unique_ptr<Job>> &jobs,
                    const std::vector<Job *> &newJobs) const;
};

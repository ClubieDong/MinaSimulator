#pragma once

#include "allocation_controller.hpp"
#include "fat_tree_resource.hpp"
#include "job.hpp"
#include "utils/graph.hpp"
#include "utils/mean_std_tracker.hpp"
#include <memory>
#include <vector>

class SmartTreeBuildingPolicy {
private:
    struct Hash {
        std::size_t operator()(const std::vector<std::pair<unsigned int, std::string_view>> &obj) const;
    };

    Graph m_ConflictGraph;
    std::vector<FatTree::AggrTree> m_AggrTrees;
    std::vector<unsigned int> m_TreeIdxToJobId;

    // Job count -> tracker
    std::unordered_map<unsigned int, MeanStdTracker> m_ScoreTrackers;
    // Job info list -> simulation result
    std::unordered_map<std::vector<std::pair<unsigned int, std::string_view>>, SimulationResult, Hash> m_SimResCache;

    unsigned int m_Count = 0;

public:
    inline static unsigned int SimulationStepCount = 3;
    inline static unsigned int MaxSharingJobCount = 30;
    inline static double Sigma = 1;
    inline static double ScoreMinStd = 0.02;
    inline static unsigned int MinTrackerCount = 10;

    const std::optional<unsigned int> MaxTreeCount;

    explicit SmartTreeBuildingPolicy(std::optional<unsigned int> maxTreeCount) : MaxTreeCount(maxTreeCount) {}

    void operator()(const FatTreeResource &resources, const std::vector<std::unique_ptr<Job>> &jobs,
                    const std::vector<Job *> &newJobs);
};

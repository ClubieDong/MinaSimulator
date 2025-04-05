#include "smart.hpp"
#include "allocation_controller.hpp"
#include "sharing_policies/smart.hpp"
#include "utils/graph.hpp"
#include "utils/mean_std_tracker.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <random>
#include <set>
#include <unordered_set>
#include <vector>

constexpr bool SHOW_TIME = false;

std::size_t
SmartTreeBuildingPolicy::Hash::operator()(const std::vector<std::pair<unsigned int, std::string_view>> &obj) const {
    std::size_t res = 0;
    for (const auto &[hostCount, modelName] : obj) {
        res ^= std::hash<unsigned int>()(hostCount);
        res ^= std::hash<std::string_view>()(modelName);
    }
    return res;
}

void SmartTreeBuildingPolicy::operator()(const FatTreeResource &resources,
                                         const std::vector<std::unique_ptr<Job>> &jobs,
                                         const std::vector<Job *> &newJobs) {
    // Get sub graph from previous conflict graph
    auto start = std::chrono::high_resolution_clock::now();
    std::unordered_map<unsigned int, Job *> jobIdMap;
    for (auto &job : jobs)
        jobIdMap[job->ID] = job.get();
    std::vector<char> nodeSet(m_ConflictGraph.GetNodeCount(), false);
    std::vector<FatTree::AggrTree> newAggrTrees;
    std::vector<unsigned int> newTreeIdxToJobId;
    for (unsigned int i = 0; i < nodeSet.size(); ++i)
        if (jobIdMap.count(m_TreeIdxToJobId[i]) > 0) {
            nodeSet[i] = true;
            newAggrTrees.push_back(std::move(m_AggrTrees[i]));
            newTreeIdxToJobId.push_back(m_TreeIdxToJobId[i]);
        }
    m_ConflictGraph = m_ConflictGraph.GetSubGraph(nodeSet);
    m_AggrTrees = std::move(newAggrTrees);
    m_TreeIdxToJobId = std::move(newTreeIdxToJobId);
    auto finish = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count();
    if (SHOW_TIME)
        std::cout << "Sub graph time: " << duration / 1000.0 << " ms\n";

    // Build aggregation tree for each new job
    start = std::chrono::high_resolution_clock::now();
    for (auto newJob : newJobs) {
        auto roots = resources.Topology->GetClosestCommonAncestors(newJob->GetHosts());
        std::vector<const FatTree::Node *> chosenRoots;
        if (MaxTreeCount && *MaxTreeCount < roots.size()) {
            thread_local std::default_random_engine engine(42);
            std::sample(roots.cbegin(), roots.cend(), std::back_inserter(chosenRoots), *MaxTreeCount, engine);
        } else
            chosenRoots = std::move(roots);
        for (auto root : chosenRoots) {
            m_AggrTrees.push_back(resources.Topology->GetAggregationTree(newJob->GetHosts(), root));
            m_TreeIdxToJobId.push_back(newJob->ID);
        }
    }
    finish = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count();
    if (SHOW_TIME)
        std::cout << "Build aggregation tree time: " << duration / 1000.0 << " ms\n";

    // Build conflict graph incrementally
    start = std::chrono::high_resolution_clock::now();
    auto oldSize = m_ConflictGraph.GetNodeCount();
    m_ConflictGraph.SetNodeCount(m_AggrTrees.size());
    for (unsigned int i = oldSize; i < m_ConflictGraph.GetNodeCount(); ++i)
        m_ConflictGraph.SetNodeWeight(i, jobIdMap[m_TreeIdxToJobId[i]]->HostCount);
    for (unsigned int i = oldSize; i < m_ConflictGraph.GetNodeCount(); ++i)
        for (unsigned int j = 0; j < i; ++j)
            if (m_TreeIdxToJobId[i] == m_TreeIdxToJobId[j] ||
                resources.CheckTreeConflict(m_AggrTrees[i], m_AggrTrees[j]))
                m_ConflictGraph.AddEdge(i, j, false);
    finish = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count();
    if (SHOW_TIME)
        std::cout << "Incremental conflict graph time: " << duration / 1000.0 << " ms\n";

    // Find maximum independent set
    start = std::chrono::high_resolution_clock::now();
    auto mis = m_ConflictGraph.CalcMaxIndependentSet();
    finish = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count();
    if (SHOW_TIME)
        std::cout << "MIS time: " << duration / 1000.0 << " ms\n";

    // Find sharing opportunities
    start = std::chrono::high_resolution_clock::now();
    std::unordered_map<unsigned int, unsigned int> treeIdxToGroupIdx;
    std::vector<std::vector<unsigned int>> sharingGroups;
    std::unordered_map<unsigned int, SimulationResult> jobSimResMap;
    std::vector<SimulationResult> sharingGroupSimRes;
    std::unordered_map<unsigned int, unsigned int> addedJobIdToTreeIdx;
    sharingGroups.reserve(mis.size());
    for (auto center : mis) {
        treeIdxToGroupIdx[center] = sharingGroups.size();
        sharingGroups.push_back({center});
        addedJobIdToTreeIdx[m_TreeIdxToJobId[center]] = center;
    }
    // (treeIdx, a list of sharing group indices that the tree is adjacent to)
    using AdjGroup = std::pair<unsigned int, std::set<unsigned int>>;
    // (A set of sharing group indices, a list of tree indices that are adjacent to the sharing groups)
    using MergeOpportunity = std::pair<std::set<unsigned int>, std::vector<unsigned int>>;
    auto getAdjGroups = [&, this] {
        std::vector<AdjGroup> adjGroups(m_ConflictGraph.GetNodeCount());
        for (unsigned int i = 0; i < adjGroups.size(); ++i)
            adjGroups[i].first = i;
        for (auto [treeIdx, groupIdx] : treeIdxToGroupIdx)
            for (auto neighbor : m_ConflictGraph.GetNeighbors(treeIdx))
                if (treeIdxToGroupIdx.count(neighbor) == 0)
                    adjGroups[neighbor].second.insert(groupIdx);
        auto end = std::remove_if(adjGroups.begin(), adjGroups.end(), [&, this](const auto &x) {
            return addedJobIdToTreeIdx.count(m_TreeIdxToJobId[x.first]) > 0;
        });
        adjGroups.erase(end, adjGroups.end());
        std::sort(adjGroups.begin(), adjGroups.end(), [](const auto &a, const auto &b) {
            if (a.second.size() != b.second.size())
                return a.second.size() < b.second.size();
            return a.second < b.second;
        });
        return adjGroups;
    };
    auto getMergeOpportunities = [this](std::vector<AdjGroup> &&adjGroups) {
        std::vector<MergeOpportunity> mergeOpportunities;
        unsigned int beginIdx = -1u;
        for (unsigned int i = 0; i <= adjGroups.size(); ++i) {
            if (beginIdx == -1u)
                beginIdx = i;
            if (i < adjGroups.size() && (i == beginIdx || adjGroups[i].second == adjGroups[beginIdx].second))
                continue;
            if (beginIdx < i) {
                // Only keep unique jobs
                std::unordered_set<unsigned int> jobIds;
                std::vector<unsigned int> treeIdxList;
                for (unsigned int j = beginIdx; j < i; ++j) {
                    auto treeIdx = adjGroups[j].first;
                    auto jobId = m_TreeIdxToJobId[treeIdx];
                    if (jobIds.count(jobId) > 0)
                        continue;
                    jobIds.insert(jobId);
                    treeIdxList.push_back(treeIdx);
                }
                mergeOpportunities.emplace_back(std::move(adjGroups[beginIdx].second), std::move(treeIdxList));
            }
            beginIdx = i;
        }
        return mergeOpportunities;
    };
    auto getMergedJobInfoList = [&, this](const std::set<unsigned int> &groups,
                                          const std::vector<unsigned int> &trees) {
        std::vector<std::pair<unsigned int, std::string_view>> jobList;
        for (auto treeIdx : trees) {
            auto job = jobIdMap[m_TreeIdxToJobId[treeIdx]];
            jobList.emplace_back(job->HostCount, job->ModelName);
        }
        for (auto groupIdx : groups)
            for (auto treeIdx : sharingGroups[groupIdx]) {
                auto job = jobIdMap[m_TreeIdxToJobId[treeIdx]];
                jobList.emplace_back(job->HostCount, job->ModelName);
            }
        std::sort(jobList.begin(), jobList.end());
        return jobList;
    };
    auto simulateBeforeMerge = [&, this](const std::set<unsigned int> &groups, const std::vector<unsigned int> &trees) {
        SimulationResult res;
        for (auto treeIdx : trees) {
            auto jobId = jobIdMap[m_TreeIdxToJobId[treeIdx]]->ID;
            assert(jobSimResMap.count(jobId) > 0);
            const auto &jobRes = jobSimResMap[jobId];
            // Total JCT is without SHARP
            res.TotalJCT += jobRes.TotalJCTWithoutSharp;
            res.TotalJCTWeighted += jobRes.TotalJCTWithoutSharpWeighted;
            res.TotalJCTWithSharp += jobRes.TotalJCTWithSharp;
            res.TotalJCTWithSharpWeighted += jobRes.TotalJCTWithSharpWeighted;
            res.TotalJCTWithoutSharp += jobRes.TotalJCTWithoutSharp;
            res.TotalJCTWithoutSharpWeighted += jobRes.TotalJCTWithoutSharpWeighted;
        }
        for (auto groupIdx : groups) {
            const auto &groupRes = sharingGroupSimRes[groupIdx];
            res.TotalJCT += groupRes.TotalJCT;
            res.TotalJCTWeighted += groupRes.TotalJCTWeighted;
            res.TotalJCTWithSharp += groupRes.TotalJCTWithSharp;
            res.TotalJCTWithSharpWeighted += groupRes.TotalJCTWithSharpWeighted;
            res.TotalJCTWithoutSharp += groupRes.TotalJCTWithoutSharp;
            res.TotalJCTWithoutSharpWeighted += groupRes.TotalJCTWithoutSharpWeighted;
        }
        return res;
    };
    auto simulateAfterMerge = [this](const std::vector<std::pair<unsigned int, std::string_view>> &jobList) {
        if (m_SimResCache.count(jobList) > 0)
            return m_SimResCache[jobList];
        auto res = AllocationController::SimulateSharingGroup(jobList, SmartSharingPolicy(), SimulationStepCount);
        m_SimResCache[jobList] = res;
        return res;
    };
    auto findBestMerge = [&]() -> std::optional<MergeOpportunity> {
        auto start = std::chrono::high_resolution_clock::now();
        auto mergeOpportunities = getMergeOpportunities(getAdjGroups());
        unsigned int bestIdx = -1u;
        double bestScoreDiff = 0.0;
        for (unsigned int i = 0; i < mergeOpportunities.size(); ++i) {
            const auto &[groups, trees] = mergeOpportunities[i];
            auto jobList = getMergedJobInfoList(groups, trees);
            if (jobList.size() > MaxSharingJobCount)
                continue;
            auto scoreBefore = simulateBeforeMerge(groups, trees).JCTScoreWeighted();
            // Estimate the score upper bound
            auto &tracker = m_ScoreTrackers[jobList.size()];
            if (tracker.Count() >= MinTrackerCount) {
                auto scoreEstimate = tracker.Mean() + Sigma * std::max(tracker.Std(), ScoreMinStd);
                if (scoreEstimate - scoreBefore < bestScoreDiff)
                    continue;
            }
            // Calculate the real score
            auto scoreAfter = simulateAfterMerge(jobList).JCTScoreWeighted();
            tracker.Update(scoreAfter);
            if (scoreAfter - scoreBefore > bestScoreDiff) {
                bestIdx = i;
                bestScoreDiff = scoreAfter - scoreBefore;
            }
        }
        auto finish = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count();
        if (SHOW_TIME)
            std::cout << "Find best merge time: " << duration / 1000.0 << " ms\n";
        if (bestIdx == -1u)
            return std::nullopt;
        return std::move(mergeOpportunities[bestIdx]);
    };
    auto performMerge = [&, this](const std::set<unsigned int> &groups, const std::vector<unsigned int> &trees) {
        auto destGroupIdx = *groups.begin();
        sharingGroupSimRes[destGroupIdx] = simulateAfterMerge(getMergedJobInfoList(groups, trees));
        for (auto groupIdx : groups) {
            if (groupIdx == destGroupIdx)
                continue;
            for (auto treeIdx : sharingGroups[groupIdx]) {
                sharingGroups[destGroupIdx].push_back(treeIdx);
                treeIdxToGroupIdx[treeIdx] = destGroupIdx;
            }
            sharingGroups[groupIdx].clear();
            sharingGroupSimRes[groupIdx].TotalJCT = std::nan("");
            sharingGroupSimRes[groupIdx].TotalJCTWeighted = std::nan("");
        }
        for (auto treeIdx : trees) {
            sharingGroups[destGroupIdx].push_back(treeIdx);
            treeIdxToGroupIdx[treeIdx] = destGroupIdx;
            addedJobIdToTreeIdx[m_TreeIdxToJobId[treeIdx]] = treeIdx;
        }
    };
    // Merge jobs that are adjacent to only one sharing group to the group
    auto adjGroups = getAdjGroups();
    bool existMergeOpportunity = false;
    for (const auto &[treeIdx, groupIdx] : adjGroups) {
        if (groupIdx.size() > 1) {
            existMergeOpportunity = true;
            break;
        }
        if (addedJobIdToTreeIdx.count(m_TreeIdxToJobId[treeIdx]) > 0)
            continue;
        treeIdxToGroupIdx[treeIdx] = *groupIdx.begin();
        sharingGroups[*groupIdx.begin()].push_back(treeIdx);
        addedJobIdToTreeIdx[m_TreeIdxToJobId[treeIdx]] = treeIdx;
    }
    if (existMergeOpportunity) {
        // Simulate each sharing group and each job
        for (unsigned int i = 0; i < sharingGroups.size(); ++i)
            sharingGroupSimRes.push_back(simulateAfterMerge(getMergedJobInfoList({i}, {})));
        for (unsigned int treeIdx = 0; treeIdx < m_ConflictGraph.GetNodeCount(); ++treeIdx) {
            auto jobId = m_TreeIdxToJobId[treeIdx];
            if (addedJobIdToTreeIdx.count(jobId) > 0 || jobSimResMap.count(jobId) > 0)
                continue;
            jobSimResMap[jobId] = simulateAfterMerge(getMergedJobInfoList({}, {treeIdx}));
        }
        // Merge all merge opportunities
        while (true) {
            auto merge = findBestMerge();
            if (!merge.has_value())
                break;
            const auto &[groups, trees] = merge.value();
            performMerge(groups, trees);
        }
        // std::vector<std::pair<unsigned int, MeanStdTracker>> x(m_ScoreTrackers.begin(), m_ScoreTrackers.end());
        // std::sort(x.begin(), x.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
        // for (const auto &[k, v] : x)
        //     std::cout << "Job count: " << k << ", count: " << v.Count() << ", mean: " << v.Mean()
        //               << ", std: " << v.Std() << "\n";
    }
    // Set aggregation trees for each job
    for (auto &job : jobs)
        if (addedJobIdToTreeIdx.count(job->ID) > 0)
            job->SetNextAggrTree(m_AggrTrees[addedJobIdToTreeIdx[job->ID]]);
        else
            job->SetNextAggrTree(std::nullopt);
    finish = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count();
    if (SHOW_TIME)
        std::cout << "Find sharing opportunities time: " << duration / 1000.0 << " ms\n\n";

    ++m_Count;
}

// TODO: Add Tracer result to paper
// TODO: change GPU speedup ratio and bandwidth used in all experiments
// TODO: weighted in all algorithms (sharing, etc)
// TODO: remove unused includes in experiment.hpp

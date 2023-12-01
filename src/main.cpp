#include "allocation_controller.hpp"
#include "data.hpp"
#include "host_allocation_policies/first.hpp"
#include "host_allocation_policies/random.hpp"
#include "host_allocation_policies/smart.hpp"
#include "sharing_policies/greedy.hpp"
#include "sharing_policies/non_sharp.hpp"
#include "sharing_policies/smart.hpp"
#include "tree_building_policies/first.hpp"
#include "tree_building_policies/random.hpp"
#include "tree_building_policies/smart.hpp"
#include "utils/graph.hpp"
#include "utils/trace.hpp"
#include <array>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

std::vector<const char *> allModelList = {
    "../data/opt-125m-4.json",    "../data/opt-125m-16.json", "../data/opt-350m-4.json",   "../data/opt-350m-16.json",
    "../data/opt-1.3b-4.json",    "../data/bert-base-4.json", "../data/bert-base-16.json", "../data/bert-large-4.json",
    "../data/bert-large-16.json", "../data/vit-base-4.json",  "../data/vit-base-16.json",  "../data/vit-large-4.json",
    "../data/vit-large-16.json",
};
std::vector<const char *> modelList = {
    "../data/opt-125m-16.json",   "../data/opt-350m-16.json", "../data/bert-base-16.json",
    "../data/bert-large-16.json", "../data/vit-base-16.json", "../data/vit-large-16.json",
};

void LargeScaleSimulation(double bandwidth, double sharpAccRatio, bool useSmartHostAllocationPolicy,
                          bool useSmartTreeBuildingPolicy, bool useSmartSharingPolicy) {
    Trace::EnableRecording = false;
    Graph::MisSolverTimeLimit = 1.0;
    Job::CalcTransmissionDuration = DurationCaculator(bandwidth, sharpAccRatio, 0.000'05);
    FatTree topology(16);
    FatTreeResource resources(topology, std::nullopt, 1);
    auto getNextJob = []() -> std::unique_ptr<Job> {
        thread_local std::default_random_engine engine(42);
        thread_local std::vector<unsigned int> hostCountList = {2, 2, 2, 2, 3, 4, 4, 4, 4, 6, 8, 8};
        thread_local std::vector<unsigned int> stepCountList = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
        std::uniform_int_distribution<std::size_t> randomModel(0, modelList.size() - 1);
        std::uniform_int_distribution<std::size_t> randomHostCount(0, hostCountList.size() - 1);
        std::uniform_int_distribution<std::size_t> randomStepCount(0, stepCountList.size() - 1);
        auto model = modelList[randomModel(engine)];
        auto hostCount = hostCountList[randomHostCount(engine)];
        auto stepCount = stepCountList[randomStepCount(engine)];
        return std::make_unique<Job>(hostCount, stepCount, ModelInfoProvider::GetModelInfo(model));
    };
    AllocationController::HostAllocationPolicy hostAllocationPolicy;
    AllocationController::TreeBuildingPolicy treeBuildingPolicy;
    AllocationController::SharingPolicy sharingPolicy;
    if (useSmartHostAllocationPolicy)
        hostAllocationPolicy = SmartHostAllocationPolicy(0.5);
    else
        hostAllocationPolicy = FirstHostAllocationPolicy();
    if (useSmartTreeBuildingPolicy)
        treeBuildingPolicy = SmartTreeBuildingPolicy(10);
    else
        treeBuildingPolicy = FirstTreeBuildingPolicy();
    if (useSmartSharingPolicy)
        sharingPolicy = SmartSharingPolicy();
    else
        sharingPolicy = GreedySharingPolicy();
    AllocationController controller(std::move(resources), std::move(getNextJob), std::move(hostAllocationPolicy),
                                    std::move(treeBuildingPolicy), std::move(sharingPolicy));
    std::optional<double> maxSimulationTime = 1000.0;
    auto result = controller.RunSimulation(maxSimulationTime, true);
    std::cout << std::setprecision(1) << std::fixed;
    std::cout << "Simulation result of " << bandwidth / 1e9 << '/' << sharpAccRatio << '/'
              << useSmartHostAllocationPolicy << '/' << useSmartTreeBuildingPolicy << '/' << useSmartSharingPolicy
              << '\n';
    std::cout << std::setprecision(6) << std::fixed;
    std::cout << "    SimulatedTime: " << result.SimulatedTime << "s\n";
    std::cout << "    ClusterUtilization: " << result.ClusterUtilization * 100 << "%\n";
    std::cout << "    JCTScore: " << result.JCTScore << '\n';
    std::cout << "    TotalHostTime: " << result.TotalHostTime << "s\n";
    std::cout << "    TotalJCT: " << result.TotalJCT << "s\n";
    std::cout << "    TotalJCTWithSharp: " << result.TotalJCTWithSharp << "s\n";
    std::cout << "    TotalJCTWithoutSharp: " << result.TotalJCTWithoutSharp << "s\n";
    std::cout << '\n';
}

void TestAccelerateEffectiveness() {
    Trace::EnableRecording = false;
    std::unordered_map<std::string, std::vector<std::pair<double, double>>> result;
    for (double bandwidth = 1e8; bandwidth <= 20e9; bandwidth += 1e8) {
        Job::CalcTransmissionDuration = DurationCaculator(bandwidth, 1.0, 0.0);
        for (auto model : modelList) {
            Job job(2, 1, ModelInfoProvider::GetModelInfo(model));
            result[model].emplace_back(bandwidth, job.StepDurationWithoutSharp);
        }
    }
    nlohmann::json jsonResult = result;
    std::ofstream file("accelerate_effectiveness.json");
    file << jsonResult;
}

void TestSharingPolicy(double bandwidth, double sharpAccRatio, const char *outputFileName) {
    Trace::EnableRecording = false;
    Job::CalcTransmissionDuration = DurationCaculator(bandwidth, sharpAccRatio, 0.000'05);
    FatTree topology(4);
    std::unordered_map<std::string, std::vector<std::vector<double>>> resultMatMap;
    for (unsigned int modelIdx1 = 0; modelIdx1 < modelList.size(); ++modelIdx1)
        for (unsigned int modelIdx2 = 0; modelIdx2 < modelList.size(); ++modelIdx2) {
            auto model1 = modelList[modelIdx1], model2 = modelList[modelIdx2];
            std::array<std::pair<std::string, AllocationController::SharingPolicy>, 2> sharingPolicyList = {
                std::pair("greedy", GreedySharingPolicy()),
                std::pair("smart", SmartSharingPolicy()),
            };
            for (auto &[sharingPolicyName, sharingPolicy] : sharingPolicyList) {
                FatTreeResource resources(topology, 1, 1);
                unsigned int jobCount = 0;
                auto getNextJob = [model1, model2, &jobCount]() -> std::unique_ptr<Job> {
                    if (jobCount >= 2)
                        return nullptr;
                    auto model = jobCount == 0 ? model1 : model2;
                    ++jobCount;
                    return std::make_unique<Job>(3, std::nullopt, ModelInfoProvider::GetModelInfo(model));
                };
                FirstHostAllocationPolicy hostAllocationPolicy;
                FirstTreeBuildingPolicy treeBuildingPolicy;
                AllocationController controller(std::move(resources), std::move(getNextJob),
                                                std::move(hostAllocationPolicy), std::move(treeBuildingPolicy),
                                                std::move(sharingPolicy));
                auto result = controller.RunSimulation(100.0, false);
                if (resultMatMap.count(sharingPolicyName) == 0)
                    resultMatMap[sharingPolicyName] = std::vector(modelList.size(), std::vector(modelList.size(), 0.0));
                resultMatMap[sharingPolicyName][modelIdx1][modelIdx2] = result.JCTScore;
            }
        }
    nlohmann::json jsonResult = resultMatMap;
    jsonResult["model_list"] = modelList;
    jsonResult["bandwidth"] = bandwidth;
    jsonResult["sharp_acc_atio"] = sharpAccRatio;
    jsonResult["average_score"] = nlohmann::json::object();
    std::cout << std::setprecision(6) << std::fixed;
    for (const auto &[name, result] : resultMatMap) {
        double jctScoreSum = 0.0;
        for (const auto &row : result)
            for (auto score : row)
                jctScoreSum += score;
        double averageJctScore = jctScoreSum / (modelList.size() * modelList.size());
        jsonResult["average_score"][name] = averageJctScore;
        std::cout << "Average JCT score of " << name << ": " << averageJctScore << '\n';
    }
    std::ofstream file(outputFileName);
    file << jsonResult;
}

void Test() {
    Trace::EnableRecording = true;
    Job::CalcTransmissionDuration = DurationCaculator(2'000'000'000, 1.0, 0.000'05);
    FatTree topology(4);
    FatTreeResource resources(topology, std::nullopt, 1);
    unsigned int jobCount = 0;
    auto getNextJob = [&jobCount]() -> std::unique_ptr<Job> {
        if (jobCount >= 1)
            return nullptr;
        ++jobCount;
        auto model = "../data/opt-350m-16.json";
        return std::make_unique<Job>(2, 100, ModelInfoProvider::GetModelInfo(model));
    };
    FirstHostAllocationPolicy hostAllocationPolicy;
    FirstTreeBuildingPolicy treeBuildingPolicy;
    GreedySharingPolicy sharingPolicy;
    AllocationController controller(std::move(resources), std::move(getNextJob), std::move(hostAllocationPolicy),
                                    std::move(treeBuildingPolicy), std::move(sharingPolicy));
    std::optional<double> maxSimulationTime = std::nullopt;
    auto result = controller.RunSimulation(maxSimulationTime, true);
    std::cout << std::setprecision(6) << std::fixed;
    std::cout << "SimulatedTime: " << result.SimulatedTime << "s\n";
    std::cout << "ClusterUtilization: " << result.ClusterUtilization * 100 << "%\n";
    std::cout << "JCTScore: " << result.JCTScore << '\n';
    std::cout << "TotalHostTime: " << result.TotalHostTime << "s\n";
    std::cout << "TotalJCT: " << result.TotalJCT << "s\n";
    std::cout << "TotalJCTWithSharp: " << result.TotalJCTWithSharp << "s\n";
    std::cout << "TotalJCTWithoutSharp: " << result.TotalJCTWithoutSharp << "s\n";
    Trace::Flush("trace.json");
}

int main() {
    LargeScaleSimulation(4'000'000'000, 1.5, false, false, false);
    LargeScaleSimulation(4'000'000'000, 1.5, false, false, true);
    LargeScaleSimulation(4'000'000'000, 1.5, false, true, false);
    LargeScaleSimulation(4'000'000'000, 1.5, false, true, true);
    LargeScaleSimulation(4'000'000'000, 1.5, true, false, false);
    LargeScaleSimulation(4'000'000'000, 1.5, true, false, true);
    LargeScaleSimulation(4'000'000'000, 1.5, true, true, false);
    LargeScaleSimulation(4'000'000'000, 1.5, true, true, true);

    // TestSharingPolicy(2'000'000'000, 1.2, "sharing_policy_2.0G_1.2.json");
    // TestSharingPolicy(3'000'000'000, 1.2, "sharing_policy_3.0G_1.2.json");
    // TestSharingPolicy(4'000'000'000, 1.2, "sharing_policy_4.0G_1.2.json");
    // TestSharingPolicy(2'000'000'000, 1.5, "sharing_policy_2.0G_1.5.json");
    // TestSharingPolicy(3'000'000'000, 1.5, "sharing_policy_3.0G_1.5.json");
    // TestSharingPolicy(4'000'000'000, 1.5, "sharing_policy_4.0G_1.5.json");
    // // TestSharingPolicy(5'000'000'000, 1.5, "sharing_policy_5.0G_1.5.json");
    // // TestSharingPolicy(5'000'000'000, 1.2, "sharing_policy_5.0G_1.2.json");
    return 0;
}

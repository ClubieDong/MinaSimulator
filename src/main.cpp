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
#include <cmath>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

std::vector<const char *> allModelList = {
    "../data/opt-125m-4.json",    "../data/opt-125m-16.json", "../data/opt-350m-4.json",   "../data/opt-350m-16.json",
    "../data/opt-1.3b-4.json",    "../data/bert-base-4.json", "../data/bert-base-16.json", "../data/bert-large-4.json",
    "../data/bert-large-16.json", "../data/vit-base-4.json",  "../data/vit-base-16.json",  "../data/vit-large-4.json",
    "../data/vit-large-16.json",
};
std::vector<const char *> modelListBs4 = {
    "../data/opt-125m-4.json",   "../data/opt-350m-4.json", "../data/opt-1.3b-4.json",  "../data/bert-base-4.json",
    "../data/bert-large-4.json", "../data/vit-base-4.json", "../data/vit-large-4.json",
};
std::vector<const char *> modelListBs16 = {
    "../data/opt-125m-16.json",   "../data/opt-350m-16.json", "../data/bert-base-16.json",
    "../data/bert-large-16.json", "../data/vit-base-16.json", "../data/vit-large-16.json",
};
const auto &modelList = modelListBs4;

void LargeScaleSimulation(double bandwidth, double sharpAccRatio, bool useSmartHostAllocationPolicy,
                          bool useSmartTreeBuildingPolicy, bool useSmartSharingPolicy) {
    Trace::EnableRecording = false;
    Job::CalcTransmissionDuration = DurationCaculator(bandwidth, sharpAccRatio, 0.000'05);
    FatTree topology(16);
    FatTreeResource resources(topology, std::nullopt, 1);
    auto getNextJob = []() -> std::pair<std::unique_ptr<Job>, double> {
        thread_local std::default_random_engine engine(42);
        thread_local std::vector<unsigned int> hostCountList = {2, 2, 2, 2, 3, 4, 4, 4, 4, 6, 8, 8};
        thread_local std::vector<unsigned int> stepCountList = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
        std::uniform_int_distribution<std::size_t> randomModel(0, modelList.size() - 1);
        std::uniform_int_distribution<std::size_t> randomHostCount(0, hostCountList.size() - 1);
        std::uniform_int_distribution<std::size_t> randomStepCount(0, stepCountList.size() - 1);
        auto model = modelList[randomModel(engine)];
        auto hostCount = hostCountList[randomHostCount(engine)];
        auto stepCount = stepCountList[randomStepCount(engine)];
        auto job = std::make_unique<Job>(hostCount, stepCount, ModelInfoProvider::GetModelInfo(model));
        return {std::move(job), 0.0};
    };
    AllocationController::HostAllocationPolicy hostAllocationPolicy;
    AllocationController::TreeBuildingPolicy treeBuildingPolicy;
    AllocationController::SharingPolicy sharingPolicy;
    if (useSmartHostAllocationPolicy)
        hostAllocationPolicy = SmartHostAllocationPolicy(0.5);
    else
        hostAllocationPolicy = FirstHostAllocationPolicy();
    if (useSmartTreeBuildingPolicy)
        treeBuildingPolicy = SmartTreeBuildingPolicy(5);
    else
        treeBuildingPolicy = FirstTreeBuildingPolicy();
    if (useSmartSharingPolicy)
        sharingPolicy = SmartSharingPolicy();
    else
        sharingPolicy = GreedySharingPolicy();
    AllocationController controller(std::move(resources), std::move(getNextJob), std::move(hostAllocationPolicy),
                                    std::move(treeBuildingPolicy), std::move(sharingPolicy));
    std::optional<double> maxSimulationTime = 200.0;
    auto result = controller.RunSimulation(maxSimulationTime, true);
    {
        static std::mutex mutex;
        std::lock_guard<std::mutex> lock(mutex);
        std::ofstream file("large_scale_result.txt", std::ios::app);
        file << std::setprecision(1) << std::fixed;
        file << "Simulation result of " << bandwidth / 1e9 << '/' << sharpAccRatio << '/'
             << useSmartHostAllocationPolicy << '/' << useSmartTreeBuildingPolicy << '/' << useSmartSharingPolicy
             << '\n';
        file << std::setprecision(6) << std::fixed;
        file << "    SimulatedTime: " << result.SimulatedTime << "s\n";
        file << "    ClusterUtilization: " << result.ClusterUtilization * 100 << "%\n";
        file << "    JCTScore: " << result.JCTScore << '\n';
        file << "    TotalHostTime: " << result.TotalHostTime << "s\n";
        file << "    TotalJCT: " << result.TotalJCT << "s\n";
        file << "    TotalJCTWithSharp: " << result.TotalJCTWithSharp << "s\n";
        file << "    TotalJCTWithoutSharp: " << result.TotalJCTWithoutSharp << "s\n";
        file << '\n';
    }
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

void TestSharingPolicy(double bandwidth, double sharpAccRatio, double job2Delay, const char *outputFileName) {
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
                auto getNextJob = [model1, model2, job2Delay, &jobCount]() -> std::pair<std::unique_ptr<Job>, double> {
                    if (jobCount >= 2)
                        return {nullptr, 0.0};
                    auto model = jobCount == 0 ? model1 : model2;
                    ++jobCount;
                    auto job = std::make_unique<Job>(3, std::nullopt, ModelInfoProvider::GetModelInfo(model));
                    return {std::move(job), jobCount == 2 ? job2Delay : 0.0};
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
        unsigned int jctScoreCount = 0;
        for (const auto &row : result)
            for (auto score : row)
                if (!std::isnan(score)) {
                    jctScoreSum += score;
                    ++jctScoreCount;
                }
        double averageJctScore = jctScoreSum / jctScoreCount;
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
    auto getNextJob = [&jobCount]() -> std::pair<std::unique_ptr<Job>, double> {
        if (jobCount >= 1)
            return {nullptr, 0.0};
        ++jobCount;
        auto model = "../data/opt-350m-16.json";
        auto job = std::make_unique<Job>(2, 100, ModelInfoProvider::GetModelInfo(model));
        return {std::move(job), 0.0};
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
    // std::thread t000([] { LargeScaleSimulation(20'000'000'000, 1.5, false, false, false); });
    // std::thread t001([] { LargeScaleSimulation(20'000'000'000, 1.5, false, false, true); });
    // std::thread t010([] { LargeScaleSimulation(20'000'000'000, 1.5, false, true, false); });
    // std::thread t011([] { LargeScaleSimulation(20'000'000'000, 1.5, false, true, true); });
    // std::thread t100([] { LargeScaleSimulation(20'000'000'000, 1.5, true, false, false); });
    // std::thread t101([] { LargeScaleSimulation(20'000'000'000, 1.5, true, false, true); });
    // std::thread t110([] { LargeScaleSimulation(20'000'000'000, 1.5, true, true, false); });
    // std::thread t111([] { LargeScaleSimulation(20'000'000'000, 1.5, true, true, true); });
    // t000.join();
    // t001.join();
    // t010.join();
    // t011.join();
    // t100.join();
    // t101.join();
    // t110.join();
    // t111.join();

    TestSharingPolicy(2'000'000'000, 1.2, 0.0, "sharing_policy_2.0G_1.2.json");
    TestSharingPolicy(3'000'000'000, 1.2, 0.0, "sharing_policy_3.0G_1.2.json");
    TestSharingPolicy(4'000'000'000, 1.2, 0.0, "sharing_policy_4.0G_1.2.json");
    TestSharingPolicy(5'000'000'000, 1.2, 0.0, "sharing_policy_5.0G_1.2.json");
    TestSharingPolicy(2'000'000'000, 1.5, 0.0, "sharing_policy_2.0G_1.5.json");
    TestSharingPolicy(3'000'000'000, 1.5, 0.0, "sharing_policy_3.0G_1.5.json");
    TestSharingPolicy(4'000'000'000, 1.5, 0.0, "sharing_policy_4.0G_1.5.json");
    TestSharingPolicy(5'000'000'000, 1.5, 0.0, "sharing_policy_5.0G_1.5.json");
    return 0;
}

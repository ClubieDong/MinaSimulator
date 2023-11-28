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
#include "utils/trace.hpp"
#include <iomanip>
#include <iostream>
#include <random>

int main() {
    Trace::EnableRecording = false;
    Job::CalcTransmissionDuration = DurationCaculator(3'517'554'342, 1.4, 0.000'05);

    FatTree topology(16);
    FatTreeResource resources(topology, std::nullopt, 1);

    auto getNextJob = []() -> std::unique_ptr<Job> {
        thread_local std::default_random_engine engine(42);
        thread_local std::vector<const char *> modelList = {
            "../data/opt-125m-4.json",   "../data/opt-125m-16.json",  "../data/opt-350m-4.json",
            "../data/opt-350m-16.json",  "../data/opt-1.3b-4.json",   "../data/bert-base-4.json",
            "../data/bert-base-16.json", "../data/bert-large-4.json", "../data/bert-large-16.json",
            "../data/vit-base-4.json",   "../data/vit-base-16.json",  "../data/vit-large-4.json",
            "../data/vit-large-16.json",
        };
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

    SmartHostAllocationPolicy hostAllocationPolicy(0.5);
    SmartTreeBuildingPolicy treeBuildingPolicy(10);
    SmartSharingPolicy sharingPolicy;

    AllocationController controller(std::move(resources), std::move(getNextJob), std::move(hostAllocationPolicy),
                                    std::move(treeBuildingPolicy), std::move(sharingPolicy));
    auto result = controller.RunSimulation(10, true);

    std::cout << std::setprecision(6) << std::fixed;
    std::cout << "SimulatedTime: " << result.SimulatedTime << "s\n";
    std::cout << "ClusterUtilization: " << result.ClusterUtilization * 100 << "%\n";
    std::cout << "JCTScore: " << result.JCTScore << '\n';
    std::cout << "TotalHostTime: " << result.TotalHostTime << "s\n";
    std::cout << "TotalJCT: " << result.TotalJCT << "s\n";
    std::cout << "TotalJCTWithSharp: " << result.TotalJCTWithSharp << "s\n";
    std::cout << "TotalJCTWithoutSharp: " << result.TotalJCTWithoutSharp << "s\n";

    Trace::Flush("trace.json");
    return 0;
}

#include "experiments.hpp"

static void Simulate(bool enableMina) {
    Job::CalcTransmissionDuration = DurationCaculator(12'500'000'000, 2.0, 0.000'05);
    ModelInfoProvider::GPUSpeedupRatio = 1.0;
    std::vector<unsigned int> hostCountList, weightList;
    for (auto [hostCount, weight] : HostCountTraces[0]) {
        hostCountList.push_back(hostCount);
        weightList.push_back(weight);
    }
    FatTree topology(16);
    FatTreeResource resources(topology, 1, std::nullopt);
    auto getNextJob = [hostCountList, weightList, jobCount = 0u]() mutable -> std::unique_ptr<Job> {
        if (jobCount >= 2000)
            return nullptr;
        ++jobCount;
        thread_local std::default_random_engine engine(42);
        thread_local std::vector<unsigned int> stepCountList = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
        std::uniform_int_distribution<std::size_t> randomModel(0, ModelList.size() - 1);
        std::discrete_distribution<std::size_t> randomHostCount(weightList.cbegin(), weightList.cend());
        std::uniform_int_distribution<std::size_t> randomStepCount(0, stepCountList.size() - 1);
        std::string_view model = ModelList[randomModel(engine)];
        auto hostCount = hostCountList[randomHostCount(engine)];
        auto stepCount = stepCountList[randomStepCount(engine)];
        return std::make_unique<Job>(model, hostCount, stepCount);
    };
    AllocationController::HostAllocationPolicy hostAllocationPolicy;
    if (enableMina)
        hostAllocationPolicy = SmartHostAllocationPolicy(0.5);
    else
        hostAllocationPolicy = FirstHostAllocationPolicy();
    FirstTreeBuildingPolicy treeBuildingPolicy(5);
    GreedySharingPolicy sharingPolicy;
    AllocationController controller(std::move(resources), std::move(getNextJob), std::move(hostAllocationPolicy),
                                    std::move(treeBuildingPolicy), std::move(sharingPolicy));
    controller.RecordClusterState = 1000;
    controller.ClusterStateOutputFile =
        enableMina ? "results/cluster_state_mina.json" : "results/cluster_state_baseline.json";
    controller.RunSimulation(std::nullopt, true);
}

void RecordClusterState() {
    Simulate(true);
    Simulate(false);
}

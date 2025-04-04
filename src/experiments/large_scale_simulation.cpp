#include "experiments.hpp"

static SimulationResult Simulate(bool enableMina, unsigned int hostTraceId) {
    std::vector<unsigned int> hostCountList, weightList;
    for (auto [hostCount, weight] : HostCountTraces[hostTraceId]) {
        hostCountList.push_back(hostCount);
        weightList.push_back(weight);
    }
    FatTree topology(16);
    FatTreeResource resources(topology, std::nullopt, 1);
    auto getNextJob = [hostCountList, weightList, jobCount = 0u]() mutable -> std::unique_ptr<Job> {
        if (jobCount >= 2000)
            return nullptr;
        ++jobCount;
        thread_local std::default_random_engine engine(42);
        thread_local std::vector<unsigned int> stepCountList = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
        std::uniform_int_distribution<std::size_t> randomModel(0, ModelList.size() - 1);
        std::discrete_distribution<std::size_t> randomHostCount(weightList.cbegin(), weightList.cend());
        std::uniform_int_distribution<std::size_t> randomStepCount(0, stepCountList.size() - 1);
        auto model = ModelList[randomModel(engine)];
        auto hostCount = hostCountList[randomHostCount(engine)];
        auto stepCount = stepCountList[randomStepCount(engine)];
        return std::make_unique<Job>(model, hostCount, stepCount);
    };
    AllocationController::HostAllocationPolicy hostAllocationPolicy;
    AllocationController::TreeBuildingPolicy treeBuildingPolicy;
    AllocationController::SharingPolicy sharingPolicy;
    if (enableMina) {
        hostAllocationPolicy = SmartHostAllocationPolicy(0.5);
        treeBuildingPolicy = SmartTreeBuildingPolicy(5);
        sharingPolicy = SmartSharingPolicy();
    } else {
        hostAllocationPolicy = FirstHostAllocationPolicy();
        treeBuildingPolicy = FirstTreeBuildingPolicy(true);
        sharingPolicy = GreedySharingPolicy();
    }
    AllocationController controller(std::move(resources), std::move(getNextJob), std::move(hostAllocationPolicy),
                                    std::move(treeBuildingPolicy), std::move(sharingPolicy));
    return controller.RunSimulation(std::nullopt, true);
}

void TestLargeScaleSimulation() {
    Job::CalcTransmissionDuration = DurationCaculator(12'500'000'000, 2.0, 0.000'05);
    ModelInfoProvider::GPUSpeedupRatio = 1.0;
    // Job::CalcTransmissionDuration = DurationCaculator(25'000'000'000, 2.5, 0.000'05);
    // ModelInfoProvider::GPUSpeedupRatio = 6.34;
    auto results = Parallel::Run<SimulationResult>( //
        [] { return Simulate(true, 0); },           //
        [] { return Simulate(false, 0); },          //
        [] { return Simulate(true, 1); },           //
        [] { return Simulate(false, 1); },          //
        [] { return Simulate(true, 2); },           //
        [] { return Simulate(false, 2); },          //
        [] { return Simulate(true, 3); },           //
        [] { return Simulate(false, 3); },          //
        [] { return Simulate(true, 4); },           //
        [] { return Simulate(false, 4); },          //
        [] { return Simulate(true, 5); },           //
        [] { return Simulate(false, 5); },          //
        [] { return Simulate(true, 6); },           //
        [] { return Simulate(false, 6); },          //
        [] { return Simulate(true, 7); },           //
        [] { return Simulate(false, 7); },          //
        [] { return Simulate(true, 8); },           //
        [] { return Simulate(false, 8); },          //
        [] { return Simulate(true, 9); },           //
        [] { return Simulate(false, 9); }           //
    );

    nlohmann::json jsonResult;
    for (auto result : results)
        jsonResult.push_back(result);
    std::ofstream file("results/large_scale_simulation.json");
    file << jsonResult;

    std::cout << '\n';
    for (unsigned int i = 0; i < results.size(); ++i) {
        std::cout << "==============================\n";
        std::cout << "Trace #" << i / 2 << ", MINA " << (i % 2 == 0 ? "enabled" : "disabled") << '\n';
        std::cout << results[i] << '\n';
    }
}

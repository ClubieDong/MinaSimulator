#include "experiments.hpp"

static SimulationResult Simulate(std::optional<unsigned int> maxTreeCount) {
    std::vector<unsigned int> hostCountList, weightList;
    for (auto [hostCount, weight] : HostCountTraces[0]) {
        hostCountList.push_back(hostCount);
        weightList.push_back(weight);
    }
    FatTree topology(16);
    FatTreeResource resources(topology, 1, std::nullopt);
    auto getNextJob = [hostCountList, weightList, jobCount = 0u]() mutable -> std::unique_ptr<Job> {
        if (jobCount >= 1000)
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
    SmartHostAllocationPolicy hostAllocationPolicy(0.5);
    SmartTreeBuildingPolicy treeBuildingPolicy(maxTreeCount);
    SmartSharingPolicy sharingPolicy;
    AllocationController controller(std::move(resources), std::move(getNextJob), std::move(hostAllocationPolicy),
                                    std::move(treeBuildingPolicy), std::move(sharingPolicy));
    return controller.RunSimulation(std::nullopt, true);
}

void TestTreeBuilding() {
    Job::CalcTransmissionDuration = DurationCaculator(12'500'000'000, 2.0, 0.000'05);
    ModelInfoProvider::GPUSpeedupRatio = 1.0;
    // MacBook Pro M1 only has 8 CPU cores, we need to run in two batches
    auto results1 = Parallel::Run<SimulationResult>( //
        [] { return Simulate(1); },                  //
        [] { return Simulate(2); },                  //
        [] { return Simulate(3); },                  //
        [] { return Simulate(4); },                  //
        [] { return Simulate(5); },                  //
        [] { return Simulate(6); }                   //
    );
    auto results2 = Parallel::Run<SimulationResult>( //
        [] { return Simulate(7); },                  //
        [] { return Simulate(8); },                  //
        [] { return Simulate(9); },                  //
        [] { return Simulate(10); },                 //
        [] { return Simulate(std::nullopt); }        //
    );
    std::vector<SimulationResult> results;
    results.reserve(results1.size() + results2.size());
    results.insert(results.end(), results1.begin(), results1.end());
    results.insert(results.end(), results2.begin(), results2.end());
    nlohmann::json jsonResult;
    std::cout << std::setprecision(6) << std::fixed;
    for (unsigned int idx = 0; idx < results.size(); ++idx) {
        const auto &res = results[idx];
        auto timeCostHostAllocation = res.TimeCostHostAllocation / res.FinishedJobCount;
        auto timeCostTreeBuilding = res.TimeCostTreeBuilding / res.FinishedJobCount;
        std::cout << "==========================\n";
        std::cout << "MaxTreeCount = " << (idx == results.size() - 1 ? "All" : std::to_string(idx + 1)) << ":\n";
        std::cout << "  Host allocation time per job: " << timeCostHostAllocation << "ms\n";
        std::cout << "  Tree building time per job: " << timeCostTreeBuilding << "ms\n";
        std::cout << "  Tree migration count=" << res.TreeMigrationCount << '\n';
        std::cout << "  JCTScoreWeighted=" << res.JCTScoreWeighted() << '\n';
        std::cout << "  SharpRatioWeighted=" << res.SharpRatioWeighted() << '\n';
        std::cout << "  SharpUtilization=" << res.SharpUtilization * 100 << "%\n";
        jsonResult.push_back(res);
    }
    std::ofstream file("results/tree_building.json");
    file << jsonResult;
}

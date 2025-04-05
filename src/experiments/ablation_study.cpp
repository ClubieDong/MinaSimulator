#include "experiments.hpp"

static SimulationResult Simulate(bool useSmartHostAllocationPolicy, bool useSmartTreeBuildingPolicy,
                                 bool useSmartSharingPolicy) {
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
        treeBuildingPolicy = FirstTreeBuildingPolicy(true);
    if (useSmartSharingPolicy)
        sharingPolicy = SmartSharingPolicy();
    else
        sharingPolicy = GreedySharingPolicy();
    AllocationController controller(std::move(resources), std::move(getNextJob), std::move(hostAllocationPolicy),
                                    std::move(treeBuildingPolicy), std::move(sharingPolicy));
    return controller.RunSimulation(std::nullopt, true);
}

void TestAblationStudy() {
    Job::CalcTransmissionDuration = DurationCaculator(12'500'000'000, 2.0, 0.000'05);
    ModelInfoProvider::GPUSpeedupRatio = 1.0;
    auto printResult = [](std::string_view name, const SimulationResult &result) {
        std::cout << name << ": ";
        std::cout << result.JCTScore() << ' ';
        std::cout << result.JCTScoreWeighted() << ' ';
        std::cout << result.SharpRatio() << ' ';
        std::cout << result.SharpRatioWeighted() << ' ';
        std::cout << result.SharpUtilization << '\n';
    };

    auto results = Parallel::Run<SimulationResult>(   //
        [] { return Simulate(false, false, false); }, //
        [] { return Simulate(false, false, true); },  //
        [] { return Simulate(false, true, false); },  //
        [] { return Simulate(false, true, true); },   //
        [] { return Simulate(true, false, false); },  //
        [] { return Simulate(true, false, true); },   //
        [] { return Simulate(true, true, false); },   //
        [] { return Simulate(true, true, true); }     //
    );

    std::cout << std::setprecision(6) << std::fixed;
    printResult("000", results[0]);
    printResult("100", results[4]);
    printResult("110", results[6]);
    printResult("111", results[7]);
    std::cout << '\n';
    printResult("011", results[3]);
    printResult("101", results[5]);
    printResult("110", results[6]);
    std::cout << '\n';
    printResult("100", results[4]);
    printResult("010", results[2]);
    printResult("001", results[1]);
}

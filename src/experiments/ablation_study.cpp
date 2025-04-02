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
    unsigned int jobCount = 0;
    auto getNextJob = [hostCountList, weightList, &jobCount]() -> std::unique_ptr<Job> {
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
        return std::make_unique<Job>(hostCount, stepCount, ModelInfoProvider::GetModelInfo(model, 1.0));
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
    // Job::CalcTransmissionDuration = DurationCaculator(12'500'000'000, 2.0, 0.000'05);
    // auto results = Parallel::Run<SimulationResult>(
    //     [] { return Simulate(false, false, false); }, [] { return Simulate(false, false, true); },
    //     [] { return Simulate(false, true, false); }, [] { return Simulate(false, true, true); },
    //     [] { return Simulate(true, false, false); }, [] { return Simulate(true, false, true); },
    //     [] { return Simulate(true, true, false); }, [] { return Simulate(true, true, true); });

    // std::cout << std::setprecision(6) << std::fixed;
    // std::cout << "000 " << results[0].JCTScore << ' ' << results[0].SharpRatio << ' ' << results[0].ClusterUtilization
    //           << ' ' << results[0].SharpUtilization << '\n';
    // std::cout << "100 " << results[4].JCTScore << ' ' << results[4].SharpRatio << ' ' << results[4].ClusterUtilization
    //           << ' ' << results[4].SharpUtilization << '\n';
    // std::cout << "110 " << results[6].JCTScore << ' ' << results[6].SharpRatio << ' ' << results[6].ClusterUtilization
    //           << ' ' << results[6].SharpUtilization << '\n';
    // std::cout << "111 " << results[7].JCTScore << ' ' << results[7].SharpRatio << ' ' << results[7].ClusterUtilization
    //           << ' ' << results[7].SharpUtilization << '\n';
    // std::cout << '\n';
    // std::cout << "111 " << results[7].JCTScore << ' ' << results[7].SharpRatio << ' ' << results[7].ClusterUtilization
    //           << ' ' << results[7].SharpUtilization << '\n';
    // std::cout << "011 " << results[3].JCTScore << ' ' << results[3].SharpRatio << ' ' << results[3].ClusterUtilization
    //           << ' ' << results[3].SharpUtilization << '\n';
    // std::cout << "101 " << results[5].JCTScore << ' ' << results[5].SharpRatio << ' ' << results[5].ClusterUtilization
    //           << ' ' << results[5].SharpUtilization << '\n';
    // std::cout << "110 " << results[6].JCTScore << ' ' << results[6].SharpRatio << ' ' << results[6].ClusterUtilization
    //           << ' ' << results[6].SharpUtilization << '\n';
    // std::cout << '\n';
    // std::cout << "100 " << results[4].JCTScore << ' ' << results[4].SharpRatio << ' ' << results[4].ClusterUtilization
    //           << ' ' << results[4].SharpUtilization << '\n';
    // std::cout << "010 " << results[2].JCTScore << ' ' << results[2].SharpRatio << ' ' << results[2].ClusterUtilization
    //           << ' ' << results[2].SharpUtilization << '\n';
    // std::cout << "001 " << results[1].JCTScore << ' ' << results[1].SharpRatio << ' ' << results[1].ClusterUtilization
    //           << ' ' << results[1].SharpUtilization << '\n';
    // std::cout << '\n';
    // std::cout << "000 " << results[0].JCTScore << ' ' << results[0].SharpRatio << ' ' << results[0].ClusterUtilization
    //           << ' ' << results[0].SharpUtilization << '\n';
    // std::cout << "111 " << results[7].JCTScore << ' ' << results[7].SharpRatio << ' ' << results[7].ClusterUtilization
    //           << ' ' << results[7].SharpUtilization << '\n';

    Job::CalcTransmissionDuration = DurationCaculator(12'500'000'000, 2.0, 0.000'05);
    auto results = Parallel::Run<SimulationResult>([] { return Simulate(true, true, true); });

    std::cout << std::setprecision(6) << std::fixed;
    std::cout << results[0].JCTScore << ' ' << results[0].SharpRatio << '\n';
}

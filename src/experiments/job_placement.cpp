#include "experiments.hpp"

static SimulationResult Simulate(const FatTree &topology, bool enableMina) {
    std::vector<unsigned int> hostCountList, weightList;
    for (auto [hostCount, weight] : HostCountTraces[0]) {
        hostCountList.push_back(hostCount);
        weightList.push_back(weight);
    }
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
    if (enableMina)
        hostAllocationPolicy = SmartHostAllocationPolicy(0.5);
    else
        hostAllocationPolicy = FirstHostAllocationPolicy();
    FirstTreeBuildingPolicy treeBuildingPolicy(true);
    GreedySharingPolicy sharingPolicy;
    AllocationController controller(std::move(resources), std::move(getNextJob), std::move(hostAllocationPolicy),
                                    std::move(treeBuildingPolicy), std::move(sharingPolicy));
    return controller.RunSimulation(std::nullopt, true);
}

void TestJobPlacement() {
    Job::CalcTransmissionDuration = DurationCaculator(12'500'000'000, 2.0, 0.000'05);
    ModelInfoProvider::GPUSpeedupRatio = 1.0;

    auto resultMina = Parallel::RunRanks<SimulationResult>(
        [](unsigned int rank) {
            FatTree tree({8, 8, 16}, {1, rank / 8 + 1, rank % 8 + 1});
            return Simulate(tree, true);
        },
        64);
    auto resultBaseline = Parallel::RunRanks<SimulationResult>(
        [](unsigned int rank) {
            FatTree tree({8, 8, 16}, {1, rank / 8 + 1, rank % 8 + 1});
            return Simulate(tree, false);
        },
        64);

    std::ofstream file("results/job_placement.json");
    file << nlohmann::json({
        {"mina", resultMina},
        {"baseline", resultBaseline},
    });

    std::cout << std::setprecision(4) << std::fixed;
    std::cout << "========= MINA JCTScore =========\n";
    for (unsigned int i = 0; i < 8; ++i) {
        for (unsigned int j = 0; j < 8; ++j)
            std::cout << resultMina[i * 8 + j].JCTScoreWeighted() << ' ';
        std::cout << '\n';
    }
    std::cout << '\n';
    std::cout << "========= Baseline JCTScore =========\n";
    for (unsigned int i = 0; i < 8; ++i) {
        for (unsigned int j = 0; j < 8; ++j)
            std::cout << resultBaseline[i * 8 + j].JCTScoreWeighted() << ' ';
        std::cout << '\n';
    }
    std::cout << '\n';
    std::cout << "========= MINA SharpRatio =========\n";
    for (unsigned int i = 0; i < 8; ++i) {
        for (unsigned int j = 0; j < 8; ++j)
            std::cout << resultMina[i * 8 + j].SharpRatioWeighted() << ' ';
        std::cout << '\n';
    }
    std::cout << '\n';
    std::cout << "========= Baseline SharpRatio =========\n";
    for (unsigned int i = 0; i < 8; ++i) {
        for (unsigned int j = 0; j < 8; ++j)
            std::cout << resultBaseline[i * 8 + j].SharpRatioWeighted() << ' ';
        std::cout << '\n';
    }
}

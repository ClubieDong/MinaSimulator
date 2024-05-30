#include "experiments.hpp"

static SimulationResult Simulate(const FatTree &topology,
                                 AllocationController::HostAllocationPolicy &&hostAllocationPolicy) {
    std::vector<unsigned int> hostCountList, weightList;
    for (auto [hostCount, weight] : HostCountTraces[0]) {
        hostCountList.push_back(hostCount);
        weightList.push_back(weight);
    }
    FatTreeResource resources(topology, std::nullopt, 1);
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
    SmartTreeBuildingPolicy treeBuildingPolicy(5);
    GreedySharingPolicy sharingPolicy;
    AllocationController controller(std::move(resources), std::move(getNextJob), std::move(hostAllocationPolicy),
                                    std::move(treeBuildingPolicy), std::move(sharingPolicy));
    return controller.RunSimulation(std::nullopt, true);
}

void TestJobPlacement() {
    Job::CalcTransmissionDuration = DurationCaculator(12'500'000'000, 2.0, 0.000'05);
    std::array ft = {
        FatTree({8, 8, 16}, {1, 1, 1}), FatTree({8, 8, 16}, {1, 2, 2}), FatTree({8, 8, 16}, {1, 3, 3}),
        FatTree({8, 8, 16}, {1, 4, 4}), FatTree({8, 8, 16}, {1, 5, 5}), FatTree({8, 8, 16}, {1, 6, 6}),
        FatTree({8, 8, 16}, {1, 7, 7}), FatTree({8, 8, 16}, {1, 8, 8}),
    };
    auto results = Parallel::Run<SimulationResult>([&ft] { return Simulate(ft[0], RandomHostAllocationPolicy()); },
                                                   [&ft] { return Simulate(ft[0], FirstHostAllocationPolicy()); },
                                                   [&ft] { return Simulate(ft[0], SmartHostAllocationPolicy(0.5)); },
                                                   [&ft] { return Simulate(ft[1], RandomHostAllocationPolicy()); },
                                                   [&ft] { return Simulate(ft[1], FirstHostAllocationPolicy()); },
                                                   [&ft] { return Simulate(ft[1], SmartHostAllocationPolicy(0.5)); },
                                                   [&ft] { return Simulate(ft[2], RandomHostAllocationPolicy()); },
                                                   [&ft] { return Simulate(ft[2], FirstHostAllocationPolicy()); },
                                                   [&ft] { return Simulate(ft[2], SmartHostAllocationPolicy(0.5)); },
                                                   [&ft] { return Simulate(ft[3], RandomHostAllocationPolicy()); },
                                                   [&ft] { return Simulate(ft[3], FirstHostAllocationPolicy()); },
                                                   [&ft] { return Simulate(ft[3], SmartHostAllocationPolicy(0.5)); },
                                                   [&ft] { return Simulate(ft[4], RandomHostAllocationPolicy()); },
                                                   [&ft] { return Simulate(ft[4], FirstHostAllocationPolicy()); },
                                                   [&ft] { return Simulate(ft[4], SmartHostAllocationPolicy(0.5)); },
                                                   [&ft] { return Simulate(ft[5], RandomHostAllocationPolicy()); },
                                                   [&ft] { return Simulate(ft[5], FirstHostAllocationPolicy()); },
                                                   [&ft] { return Simulate(ft[5], SmartHostAllocationPolicy(0.5)); },
                                                   [&ft] { return Simulate(ft[6], RandomHostAllocationPolicy()); },
                                                   [&ft] { return Simulate(ft[6], FirstHostAllocationPolicy()); },
                                                   [&ft] { return Simulate(ft[6], SmartHostAllocationPolicy(0.5)); },
                                                   [&ft] { return Simulate(ft[7], RandomHostAllocationPolicy()); },
                                                   [&ft] { return Simulate(ft[7], FirstHostAllocationPolicy()); },
                                                   [&ft] { return Simulate(ft[7], SmartHostAllocationPolicy(0.5)); });
    std::cout << std::setprecision(3) << std::fixed;
    nlohmann::json jsonResult;
    for (unsigned int i = 0; i < 8; ++i) {
        std::array res = {results[i * 3 + 0].JCTScore,
                          results[i * 3 + 1].JCTScore,
                          results[i * 3 + 2].JCTScore};
        jsonResult.push_back(res);
        std::cout << "Ratio=8:" << i + 1 << ": ";
        for (auto j : res)
            std::cout << j << ' ';
        std::cout << '\n';
    }
    std::ofstream file("results/job_placement.json");
    file << jsonResult;
}

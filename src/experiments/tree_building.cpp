#include "experiments.hpp"

static SimulationResult Simulate(std::optional<unsigned int> maxTreeCount) {
    std::vector<unsigned int> hostCountList, weightList;
    for (auto [hostCount, weight] : HostCountTraces[0]) {
        hostCountList.push_back(hostCount);
        weightList.push_back(weight);
    }
    FatTree topology(16);
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
    SmartHostAllocationPolicy hostAllocationPolicy(0.5);
    SmartTreeBuildingPolicy treeBuildingPolicy(maxTreeCount);
    GreedySharingPolicy sharingPolicy;
    AllocationController controller(std::move(resources), std::move(getNextJob), std::move(hostAllocationPolicy),
                                    std::move(treeBuildingPolicy), std::move(sharingPolicy));
    return controller.RunSimulation(std::nullopt, true);
}

void TestTreeBuilding() {
    Job::CalcTransmissionDuration = DurationCaculator(12'500'000'000, 2.0, 0.000'05);
    auto results = Parallel::Run<SimulationResult>(
        [] { return Simulate(1); },
        [] { return Simulate(2); },
        [] { return Simulate(3); },
        [] { return Simulate(4); },
        [] { return Simulate(5); },
        [] { return Simulate(6); },
        [] { return Simulate(7); },
        [] { return Simulate(8); },
        [] { return Simulate(9); },
        [] { return Simulate(10); },
        [] { return Simulate(std::nullopt); }
    );
    nlohmann::json jsonResult;
    std::cout << std::setprecision(6) << std::fixed;
    for (unsigned int idx = 0; idx < results.size(); ++idx) {
        const auto &res = results[idx];
        auto timeCostHostAllocation = res.TimeCostHostAllocation / res.FinishedJobCount;
        auto timeCostTreeBuilding = res.TimeCostTreeBuilding / res.FinishedJobCount;
        std::cout << "MaxTreeCount=" << idx + 1 << ":\n";
        std::cout << "  HostAllocation: " << timeCostHostAllocation << "ms ";
        std::cout << "  TreeBuilding: " << timeCostTreeBuilding << "ms ";
        std::cout << "  JCTScore: " << res.JCTScore << ' ';
        std::cout << "  SharpRatio: " << res.SharpRatio << '\n';
        std::cout << "  TreeMigrationCount: " << res.TreeMigrationCount << '\n';
        jsonResult.push_back({
            {"MaxTreeCount", idx + 1},
            {"TimeCostHostAllocation", timeCostHostAllocation},
            {"TimeCostTreeBuilding", timeCostTreeBuilding},
            {"JCTScore", res.JCTScore},
            {"SharpRatio", res.SharpRatio},
            {"TreeMigrationCount", res.TreeMigrationCount},
        });
    }
    jsonResult[jsonResult.size() - 1]["MaxTreeCount"] = "All";
    std::ofstream file("tree_building.json");
    file << jsonResult;
}

#include "experiments.hpp"

void TestSharingOverhead() {
    Job::CalcTransmissionDuration = DurationCaculator(12'500'000'000, 2.0, 0.000'05);
    SharingGroup::RecordSharingOverhead = true;
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
    SmartTreeBuildingPolicy treeBuildingPolicy(5);
    SmartSharingPolicy sharingPolicy;
    AllocationController controller(std::move(resources), std::move(getNextJob), std::move(hostAllocationPolicy),
                                    std::move(treeBuildingPolicy), std::move(sharingPolicy));
    auto result = controller.RunSimulation(std::nullopt, true);
    std::cout << result.ConsensusFrequency << '\n';
    std::cout << SharingGroup::SharingPolicyCallCount << '\n';
    std::cout << SharingGroup::SharingPolicyOverhead << '\n';
}

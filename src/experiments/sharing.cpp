#include "experiments.hpp"

static constexpr unsigned int StepCount = 100;

void TestSharing() {
    Job::CalcTransmissionDuration = DurationCaculator(12'500'000'000, 1.5, 0.000'05);
    ModelInfoProvider::GPUSpeedupRatio = 1.5;
    unsigned int modelCount = ModelList.size();

    auto res2Jobs = Parallel::RunRanks<double>(
        [modelCount](unsigned int rank) {
            std::vector<std::pair<unsigned int, std::string_view>> jobList = {
                {2, ModelList[rank / modelCount]}, //
                {2, ModelList[rank % modelCount]}, //
            };
            auto result = AllocationController::SimulateSharingGroup(jobList, SmartSharingPolicy(), StepCount);
            std::cout << "Simulation #" << rank + 1 << " of " << modelCount * modelCount << "finished\n";
            return result.JCTScoreWeighted();
        },
        modelCount * modelCount);

    auto res3Jobs = Parallel::RunRanks<double>(
        [modelCount](unsigned int rank) {
            std::vector<std::pair<unsigned int, std::string_view>> jobList = {
                {2, ModelList[rank / (modelCount * modelCount)]}, //
                {2, ModelList[(rank / modelCount) % modelCount]}, //
                {2, ModelList[rank % modelCount]},                //
            };
            auto result = AllocationController::SimulateSharingGroup(jobList, SmartSharingPolicy(), StepCount);
            std::cout << "Simulation #" << rank + 1 << " of " << modelCount * modelCount * modelCount << "finished\n";
            return result.JCTScoreWeighted();
        },
        modelCount * modelCount * modelCount);

    auto res4Jobs = Parallel::RunRanks<double>(
        [modelCount](unsigned int rank) {
            std::vector<std::pair<unsigned int, std::string_view>> jobList = {
                {2, ModelList[rank / (modelCount * modelCount * modelCount)]},   //
                {2, ModelList[(rank / (modelCount * modelCount)) % modelCount]}, //
                {2, ModelList[(rank / modelCount) % modelCount]},                //
                {2, ModelList[rank % modelCount]},                               //
            };
            auto result = AllocationController::SimulateSharingGroup(jobList, SmartSharingPolicy(), StepCount);
            std::cout << "Simulation #" << rank + 1 << " of " << modelCount * modelCount * modelCount * modelCount
                      << "finished\n";
            return result.JCTScoreWeighted();
        },
        modelCount * modelCount * modelCount * modelCount);

    std::ofstream file("results/sharing.json");
    file << nlohmann::json({
        {"model_list", ModelList},
        {"result_2_jobs", res2Jobs},
        {"result_3_jobs", res3Jobs},
        {"result_4_jobs", res4Jobs},
    });

    std::cout << std::setprecision(4) << std::fixed;
    double jctScoreSum = 0.0;
    for (unsigned int i = 0; i < modelCount; ++i) {
        for (unsigned int j = 0; j < modelCount; ++j) {
            jctScoreSum += res2Jobs[i * modelCount + j];
            std::cout << res2Jobs[i * modelCount + j] << ' ';
        }
        std::cout << '\n';
    }
    double averageJctScore = jctScoreSum / (modelCount * modelCount);
    std::cout << "Average JCT score: " << averageJctScore << '\n';
}

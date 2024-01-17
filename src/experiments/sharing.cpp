#include "experiments.hpp"

void TestSharing() {
    Job::CalcTransmissionDuration = DurationCaculator(12'500'000'000, 1.5, 0.000'05);
    FatTree topology(4);
    std::vector<std::vector<double>> resultMat(ModelList.size(), std::vector(ModelList.size(), 0.0));
    for (unsigned int modelIdx1 = 0; modelIdx1 < ModelList.size(); ++modelIdx1)
        for (unsigned int modelIdx2 = 0; modelIdx2 < ModelList.size(); ++modelIdx2) {
            std::cout << "Running simulation #" << modelIdx1 * ModelList.size() + modelIdx2 + 1 << " of "
                      << ModelList.size() * ModelList.size() << '\n';
            auto model1 = ModelList[modelIdx1], model2 = ModelList[modelIdx2];
            FatTreeResource resources(topology, 1, 1);
            unsigned int jobCount = 0;
            auto getNextJob = [=, &jobCount]() -> std::unique_ptr<Job> {
                if (jobCount >= 2)
                    return nullptr;
                auto model = jobCount == 0 ? model1 : model2;
                ++jobCount;
                return std::make_unique<Job>(3, std::nullopt, ModelInfoProvider::GetModelInfo(model, 1.5));
            };
            FirstHostAllocationPolicy hostAllocationPolicy;
            FirstTreeBuildingPolicy treeBuildingPolicy;
            SmartSharingPolicy sharingPolicy;
            AllocationController controller(std::move(resources), std::move(getNextJob),
                                            std::move(hostAllocationPolicy), std::move(treeBuildingPolicy),
                                            std::move(sharingPolicy));
            auto result = controller.RunSimulation(1000.0, false);
            resultMat[modelIdx1][modelIdx2] = result.JCTScore;
        }
    nlohmann::json jsonResult;
    jsonResult["model_list"] = ModelList;
    jsonResult["result"] = resultMat;
    std::ofstream file("sharing.json");
    file << jsonResult;
    std::cout << std::setprecision(6) << std::fixed;
    double jctScoreSum = 0.0;
    unsigned int jctScoreCount = 0;
    for (const auto &row : resultMat)
        for (auto score : row)
            if (!std::isnan(score) && !std::isinf(score)) {
                jctScoreSum += score;
                ++jctScoreCount;
            }
    double averageJctScore = jctScoreSum / jctScoreCount;
    std::cout << "Average JCT score: " << averageJctScore << '\n';
}

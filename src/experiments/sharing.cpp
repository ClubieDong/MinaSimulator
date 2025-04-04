#include "experiments.hpp"

void TestSharing() {
    Job::CalcTransmissionDuration = DurationCaculator(12'500'000'000, 1.5, 0.000'05);
    ModelInfoProvider::GPUSpeedupRatio = 1.5;
    std::vector<std::vector<double>> resultMat(ModelList.size(), std::vector(ModelList.size(), 0.0));
    for (unsigned int modelIdx1 = 0; modelIdx1 < ModelList.size(); ++modelIdx1)
        for (unsigned int modelIdx2 = 0; modelIdx2 < ModelList.size(); ++modelIdx2) {
            std::cout << "Running simulation #" << modelIdx1 * ModelList.size() + modelIdx2 + 1 << " of "
                      << ModelList.size() * ModelList.size() << '\n';
            std::vector<std::pair<unsigned int, std::string_view>> jobList = {{2, ModelList[modelIdx1]},
                                                                              {2, ModelList[modelIdx2]}};
            SmartSharingPolicy sharingPolicy;
            auto result = AllocationController::SimulateSharingGroup(jobList, std::move(sharingPolicy), 1000);
            // TODO: or weighted JCT score?
            resultMat[modelIdx1][modelIdx2] = result.JCTScore();
        }
    nlohmann::json jsonResult;
    jsonResult["model_list"] = ModelList;
    jsonResult["result"] = resultMat;
    std::ofstream file("results/sharing.json");
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

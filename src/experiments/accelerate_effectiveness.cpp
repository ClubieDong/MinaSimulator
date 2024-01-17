#include "experiments.hpp"

void TestAccelerateEffectiveness() {
    std::unordered_map<std::string, std::vector<std::pair<double, double>>> result;
    for (double bandwidth = 1e8; bandwidth <= 20e9; bandwidth += 1e8) {
        Job::CalcTransmissionDuration = DurationCaculator(bandwidth, 1.0, 0.0);
        for (auto model : ModelListBs4) {
            Job job(2, 1, ModelInfoProvider::GetModelInfo(model, 1.0));
            result[model].emplace_back(bandwidth, job.StepDurationWithoutSharp);
        }
        for (auto model : ModelListBs16) {
            Job job(2, 1, ModelInfoProvider::GetModelInfo(model, 1.0));
            result[model].emplace_back(bandwidth, job.StepDurationWithoutSharp);
        }
    }
    nlohmann::json jsonResult = result;
    std::ofstream file("accelerate_effectiveness.json");
    file << jsonResult;
}

#include "experiments.hpp"

void TestAccelerateEffectiveness() {
    std::unordered_map<std::string, std::vector<std::pair<double, double>>> result;
    for (double bandwidth = 1e8; bandwidth <= 20e9; bandwidth += 1e8) {
        Job::CalcTransmissionDuration = DurationCaculator(bandwidth, 1.0, 0.0);
        ModelInfoProvider::GPUSpeedupRatio = 1.0;
        for (const auto &model : ModelListBs4) {
            Job job(model, 2, 1);
            result[model].emplace_back(bandwidth, job.StepDurationWithoutSharp);
        }
        for (const auto &model : ModelListBs16) {
            Job job(model, 2, 1);
            result[model].emplace_back(bandwidth, job.StepDurationWithoutSharp);
        }
    }
    nlohmann::json jsonResult = result;
    std::ofstream file("results/accelerate_effectiveness.json");
    file << jsonResult;
}

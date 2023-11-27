#include "data.hpp"
#include <nlohmann/json.hpp>
#include <fstream>

double DurationCaculator::operator()(CommOp::Type opType, unsigned long long messageSize, bool useSharp,
                                     unsigned int hostCount) const {
    if (hostCount == 1)
        return Latency;
    auto bandwidth = Bandwidth * (useSharp && opType == CommOp::Type::AllReduce ? SharpAccRatio : 1);
    return (Latency + messageSize / bandwidth) * 1'000'000;
}

std::vector<CommOpGroup> ModelInfoProvider::GetModelInfo(const char *modelInfoPath) {
    std::ifstream file(modelInfoPath);
    auto modelInfo = nlohmann::json::parse(file);
    std::vector<CommOpGroup> result(1);
    auto &opGroup = result.front();
    opGroup.SyncTime = modelInfo["duration"].get<double>() * 1'000'000;
    for (const auto &op : modelInfo["allreduces"]) {
        auto start = op["start"].get<double>() * 1'000'000;
        auto size = op["size"].get<unsigned long long>();
        opGroup.CommOps.emplace_back(start, size, CommOp::Type::AllReduce);
    }
    return result;
}

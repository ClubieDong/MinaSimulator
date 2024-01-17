#include "data.hpp"
#include <fstream>
#include <nlohmann/json.hpp>

double DurationCaculator::operator()(CommOp::Type opType, unsigned long long messageSize, bool useSharp,
                                     unsigned int hostCount) const {
    if (hostCount == 1)
        return Latency;
    auto bandwidth = Bandwidth * (useSharp && opType == CommOp::Type::AllReduce ? SharpAccRatio : 1);
    return Latency + messageSize / bandwidth;
}

std::vector<CommOpGroup> ModelInfoProvider::GetModelInfo(const char *modelInfoPath, double gpuSpeedupRatio) {
    std::scoped_lock lock(m_CacheMtx);
    if (m_ModelInfoCache.count(modelInfoPath) == 0) {
        std::ifstream file(modelInfoPath);
        m_ModelInfoCache[modelInfoPath] = nlohmann::json::parse(file);
    }
    const auto &modelInfo = m_ModelInfoCache[modelInfoPath];
    std::vector<CommOpGroup> result(1);
    auto &opGroup = result.front();
    opGroup.SyncTime = modelInfo["duration"].get<double>() / gpuSpeedupRatio;
    for (const auto &op : modelInfo["allreduces"]) {
        double start = op["start"].get<double>() / gpuSpeedupRatio;
        unsigned long long size = op["size"];
        opGroup.CommOps.emplace_back(start, size, CommOp::Type::AllReduce);
    }
    return result;
}

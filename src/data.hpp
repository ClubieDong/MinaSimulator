#pragma once

#include "job.hpp"
#include <mutex>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>

class DurationCaculator {
public:
    const double Bandwidth;     // In byte per second
    const double SharpAccRatio; // Should be greater than one
    const double Latency;       // In second

    explicit DurationCaculator(double bandwidth, double sharpAccRatio, double latency)
        : Bandwidth(bandwidth), SharpAccRatio(sharpAccRatio), Latency(latency) {}

    double operator()(CommOp::Type opType, unsigned long long messageSize, bool useSharp, unsigned int hostCount) const;
};

class ModelInfoProvider {
private:
    inline static std::mutex m_CacheMtx;
    inline static std::unordered_map<std::string_view, nlohmann::json> m_ModelInfoCache;

public:
    inline static double GPUSpeedupRatio = 1.0;

    static std::vector<CommOpGroup> GetModelInfo(std::string_view modelName);
};

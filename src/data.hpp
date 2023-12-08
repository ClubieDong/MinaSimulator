#pragma once

#include "job.hpp"
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
public:
    static std::vector<CommOpGroup> GetModelInfo(const char *modelName, double gpuSpeedupRatio);
};

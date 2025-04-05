#include "experiments.hpp"

void OutputTraces() {
    std::ofstream file("results/traces.json");
    file << nlohmann::json(HostCountTraces);
}

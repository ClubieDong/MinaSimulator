#pragma once

#include "allocation_controller.hpp"
#include "data.hpp"
#include "host_allocation_policies/first.hpp"
#include "host_allocation_policies/random.hpp"
#include "host_allocation_policies/smart.hpp"
#include "sharing_policies/greedy.hpp"
#include "sharing_policies/non_sharp.hpp"
#include "sharing_policies/smart.hpp"
#include "tree_building_policies/first.hpp"
#include "tree_building_policies/random.hpp"
#include "tree_building_policies/smart.hpp"
#include "utils/graph.hpp"
#include "utils/parallel.hpp"
#include "utils/trace.hpp"
#include <array>
#include <cmath>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

inline static std::vector<std::string> AllModelList = {
    "traces/opt-125m-4.json",    "traces/opt-125m-16.json", "traces/opt-350m-4.json",   "traces/opt-350m-16.json",
    "traces/opt-1.3b-4.json",    "traces/bert-base-4.json", "traces/bert-base-16.json", "traces/bert-large-4.json",
    "traces/bert-large-16.json", "traces/vit-base-4.json",  "traces/vit-base-16.json",  "traces/vit-large-4.json",
    "traces/vit-large-16.json",
};
inline static std::vector<std::string> ModelListBs4 = {
    "traces/opt-125m-4.json",   "traces/opt-350m-4.json", "traces/opt-1.3b-4.json",  "traces/bert-base-4.json",
    "traces/bert-large-4.json", "traces/vit-base-4.json", "traces/vit-large-4.json",
};
inline static std::vector<std::string> ModelListBs16 = {
    "traces/opt-125m-16.json",   "traces/opt-350m-16.json", "traces/bert-base-16.json",
    "traces/bert-large-16.json", "traces/vit-base-16.json", "traces/vit-large-16.json",
};
inline static const auto &ModelList = ModelListBs4;

inline static std::vector<std::vector<std::pair<unsigned int, unsigned int>>> HostCountTraces = {
    {{1, 10910}, {2, 9719}, {4, 16374}, {8, 23311}, {16, 23936}, {32, 13268}, {48, 2089}, {64, 329}, {128, 61}},
    {{1, 26464}, {2, 19460}, {4, 24834}, {8, 20690}, {16, 7818}, {32, 725}, {48, 5}},
    {{1, 44237}, {2, 14757}, {4, 18831}, {8, 15689}, {16, 5929}, {32, 550}, {48, 4}},
    {{1, 41606}, {2, 15453}, {4, 19720}, {8, 16430}, {16, 6208}, {32, 576}, {48, 4}},
    {{1, 36941}, {2, 16688}, {4, 21295}, {8, 17742}, {16, 6704}, {32, 622}, {48, 4}},
    {{1, 26464}, {2, 19460}, {4, 24834}, {8, 20690}, {16, 7818}, {32, 725}, {48, 5}},
    {{2, 36941}, {4, 28960}, {8, 24127}, {16, 9117}, {32, 846}, {48, 6}},
    {{2, 26464}, {4, 33771}, {8, 28136}, {16, 10632}, {32, 986}, {48, 7}},
    {{4, 26464}, {8, 52033}, {16, 19663}, {32, 1824}, {48, 13}},
    {{8, 26464}, {16, 67247}, {32, 6241}, {48, 45}},
};

void TestTreeConflicts();
void TestAccelerateEffectiveness();
void OutputTraces();
void TestLargeScaleSimulation();
void TestAblationStudy();
void TestJobPlacement();
void TestTreeBuilding();
void TestSharing();
void TestSharingOverhead();

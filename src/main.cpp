#include "allocation_controller.hpp"
#include "host_allocation_policies/first.hpp"
#include "host_allocation_policies/random.hpp"
#include "host_allocation_policies/smart.hpp"
#include "sharing_policies/greedy.hpp"
#include "sharing_policies/non_sharp.hpp"
#include "sharing_policies/smart.hpp"
#include "tree_building_policies/first.hpp"
#include "tree_building_policies/random.hpp"
#include "tree_building_policies/smart.hpp"
#include "utils/trace.hpp"
#include <random>

int main() {
    Trace::EnableRecording = true;
    Job::CalcTransmissionDuration = [](CommOp::Type, unsigned long long messageSize, bool useSharp,
                                       unsigned int) -> double {
        constexpr double bandwidth = 1000; // In Byte / microsecond
        constexpr double sharpAccRatio = 0.6;
        return messageSize / bandwidth * (useSharp ? sharpAccRatio : 1);
    };

    FatTree topology(4);
    FatTreeResource resources(topology, std::nullopt, 1);

    auto getNextJob = []() -> std::unique_ptr<Job> {
        static unsigned int count = 0;
        if (count == 2)
            return nullptr;
        ++count;
        std::vector<CommOpGroup> groups = {{{}, 330'000}};
        for (unsigned int i = 0; i < 12; ++i)
            groups[0].CommOps.emplace_back(150'000 + i * 15'000, 16'000'000, CommOp::Type::AllReduce);
        groups[0].CommOps.emplace_back(150'000 + 12 * 15'000, 5 * 16'000'000, CommOp::Type::AllReduce);
        thread_local std::default_random_engine engine(std::random_device{}());
        thread_local std::vector<unsigned int> hostCountList = {3};
        thread_local std::vector<unsigned int> stepCountList = {100};
        std::uniform_int_distribution<std::size_t> randomHostCount(0, hostCountList.size() - 1);
        std::uniform_int_distribution<std::size_t> randomStepCount(0, stepCountList.size() - 1);
        auto hostCount = hostCountList[randomHostCount(engine)];
        auto stepCount = stepCountList[randomStepCount(engine)];
        return std::make_unique<Job>(hostCount, stepCount, std::move(groups));
    };

    SmartHostAllocationPolicy hostAllocationPolicy;
    SmartTreeBuildingPolicy treeBuildingPolicy;
    SmartSharingPolicy sharingPolicy;

    AllocationController controller(std::move(resources), std::move(getNextJob), std::move(hostAllocationPolicy),
                                    std::move(treeBuildingPolicy), std::move(sharingPolicy));
    controller.RunSimulation();

    Trace::Flush("trace.json");
    return 0;
}

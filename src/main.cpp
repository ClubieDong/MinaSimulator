#include "allocation_controller.hpp"
#include "host_allocation_policies/first.hpp"
#include "sharing_policies/non_sharp.hpp"
#include "tree_building_policies/none.hpp"
#include "utils/trace.hpp"
#include <cassert>

int main() {
    Trace::EnableRecording = true;
    Job::CalcTransmissionDuration = [](CommOp::Type, unsigned long long messageSize, bool useSharp,
                                       unsigned int) -> double {
        if (useSharp)
            return messageSize / 2;
        return messageSize;
    };

    FatTree topology(4);
    FatTreeResource resources(topology, 1, 1);

    auto getNextJob = []() -> std::unique_ptr<Job> {
        static unsigned int count = 0;
        if (count == 10)
            return nullptr;
        ++count;
        std::vector<CommOpGroup> groups = {
            {{{0, 4, CommOp::Type::AllReduce}, {5, 4, CommOp::Type::AllReduce}, {10, 4, CommOp::Type::AllReduce}}, 15},
        };
        return std::make_unique<Job>(4, 2, std::move(groups));
    };
    FirstHostAllocationPolicy hostAllocationPolicy;
    NoneTreeBuildingPolicy treeBuildingPolicy;
    NonSharpSharingPolicy sharingPolicy;

    AllocationController controller(std::move(resources), std::move(getNextJob), std::move(hostAllocationPolicy),
                                    std::move(treeBuildingPolicy), std::move(sharingPolicy));
    controller.RunSimulation();

    Trace::Flush("trace.json");
    return 0;
}

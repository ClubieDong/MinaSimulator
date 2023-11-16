#include "job.hpp"
#include "sharing_group.hpp"
#include "sharing_policies/non_sharp.hpp"
#include "trace.hpp"
#include <cassert>
#include <iostream>

int main() {
    Trace::EnableRecording = true;
    Job::CalcTransmissionDuration = [](CommOp::Type, unsigned long long messageSize, bool useSharp,
                                       unsigned int) -> double {
        if (useSharp)
            return messageSize / 2;
        return messageSize;
    };

    std::vector<CommOpGroup> groups1 = {
        {{{0, 4, CommOp::Type::AllReduce}, {5, 4, CommOp::Type::AllReduce}, {10, 4, CommOp::Type::AllReduce}}, 15},
    };
    Job job1(4, 2, std::move(groups1));

    std::vector<CommOpGroup> groups2 = {
        {{{0, 10, CommOp::Type::AllReduce}, {3, 10, CommOp::Type::AllReduce}}, 1},
    };
    Job job2(4, 3, std::move(groups2));

    std::vector<Job *> jobs = {&job1, &job2};
    SharingGroup sharingGroup(std::move(jobs), NonSharpSharingPolicy());

    double now = 5.0;
    while (true) {
        auto [nextTime, job] = sharingGroup.GetNextEvent(now);
        assert(nextTime >= now);
        now = nextTime;
        auto res = sharingGroup.RunNextEvent(now, job);
        if (res)
            break;
    }

    Trace::Flush("trace.json");
    return 0;
}

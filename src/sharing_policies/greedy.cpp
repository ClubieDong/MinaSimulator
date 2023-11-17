#include "greedy.hpp"

CommOpScheduleResult GreedySharpSharingPolicy::operator()(const SharingGroup &sharingGroup, const Job &job,
                                                          double) const {
    auto resources = sharingGroup.GetFatTreeResources();
    const auto &aggrTree = job.GetCurrentAggrTree();
    bool useSharp = aggrTree && !resources->CheckTreeConflict(*aggrTree);
    return CommOpScheduleResult(useSharp, job.GetCurrentCommOp().MessageSize);
}

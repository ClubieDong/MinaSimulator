#include "greedy.hpp"
#include <cassert>

CommOpScheduleResult GreedySharpSharingPolicy::operator()(const SharingGroup &sharingGroup, const Job &job,
                                                          double) const {
    assert(!job.IsFinished());
    assert(!job.IsRunning());
    const auto &aggrTree = job.GetCurrentAggrTree();
    auto resources = sharingGroup.GetFatTreeResources();
    bool useSharp = aggrTree && !resources->CheckTreeConflict(*aggrTree);
    return CommOpScheduleResult(useSharp);
}

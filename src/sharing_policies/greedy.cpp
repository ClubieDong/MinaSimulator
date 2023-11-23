#include "greedy.hpp"
#include <cassert>

CommOpScheduleResult GreedySharingPolicy::operator()(const SharingGroup &sharingGroup, const Job &job,
                                                          double) const {
    assert(!job.IsFinished());
    assert(!job.IsRunning());
    const auto &aggrTree = job.GetCurrentAggrTree();
    auto resources = sharingGroup.GetFatTreeResources();
    bool useSharp = aggrTree && !resources->CheckTreeConflict(*aggrTree);
    return CommOpScheduleResult(useSharp);
}

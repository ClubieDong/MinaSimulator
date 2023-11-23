#include "smart.hpp"
#include "utils/trace.hpp"
#include <cassert>

CommOpScheduleResult SmartSharingPolicy::operator()(const SharingGroup &sharingGroup, const Job &job,
                                                         double now) const {
    if (!sharingGroup.CanUseSharp(job))
        return CommOpScheduleResult(false);
    auto thisOpInfo = job.GetNextCommOpInfo(now);
    assert(thisOpInfo);
    auto thisPriority = job.GetNextCommOpPriority(*thisOpInfo);
    auto thisRatio = thisPriority / (thisOpInfo->OpStartTime + thisOpInfo->DurationWithSharp - now);
    for (auto otherJob : sharingGroup.Jobs) {
        if (otherJob == &job)
            continue;
        auto otherOpInfo = otherJob->GetNextCommOpInfo(now);
        if (!otherOpInfo)
            continue;
        if (otherOpInfo->OpStartTime >= now + thisOpInfo->DurationWithSharp)
            continue;
        auto otherPriority = otherJob->GetNextCommOpPriority(*otherOpInfo);
        auto otherRatio = otherPriority / (otherOpInfo->OpStartTime + otherOpInfo->DurationWithSharp - now);
        if (otherRatio > thisRatio)
            return CommOpScheduleResult(false);
    }
    return CommOpScheduleResult(true);
}

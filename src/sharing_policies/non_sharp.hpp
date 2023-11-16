#pragma once

#include "job.hpp"
#include "sharing_group.hpp"

class NonSharpSharingPolicy {
public:
    CommOpScheduleResult operator()(const SharingGroup &, const Job &job, double) const {
        return CommOpScheduleResult(false, job.GetCurrentCommOp().MessageSize);
    }
};

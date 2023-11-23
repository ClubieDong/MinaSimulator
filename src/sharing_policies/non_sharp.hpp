#pragma once

#include "job.hpp"
#include "sharing_group.hpp"

class NonSharpSharingPolicy {
public:
    CommOpScheduleResult operator()(const SharingGroup &, const Job &, double) const {
        return CommOpScheduleResult(false);
    }
};

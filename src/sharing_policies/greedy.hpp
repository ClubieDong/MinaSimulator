#pragma once

#include "job.hpp"
#include "sharing_group.hpp"

class GreedySharpSharingPolicy {
public:
    CommOpScheduleResult operator()(const SharingGroup &sharingGroup, const Job &job, double now) const;
};

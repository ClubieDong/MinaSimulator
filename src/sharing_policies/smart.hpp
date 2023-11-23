#pragma once

#include "job.hpp"
#include "sharing_group.hpp"

class SmartSharingPolicy {
public:
    CommOpScheduleResult operator()(const SharingGroup &sharingGroup, const Job &job, double now) const;
};

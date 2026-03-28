#pragma once

#include "stats/acceleration_stats.h"
#include "stats/raytracing_stats.h"

namespace diplodocus {

struct Stats {
    RaytracingStats rt_stats;
    AccelerationStats accel_stats;
};

}  // namespace diplodocus

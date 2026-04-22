#pragma once

#include "stats/construction_stats.h"
#include "stats/raytracing_stats.h"

namespace diplodocus {

struct Stats {
    RaytracingStats rt_stats;
    ConstructionStats construction_stats;
};

}  // namespace diplodocus

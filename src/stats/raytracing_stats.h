#pragma once

namespace diplodocus {

struct RaytracingStats {
    double raytracing_time = 0.0;
    int primary_ray_count = 0;
    int secondary_ray_count = 0;
    int shadow_ray_count = 0;
};

}  // namespace diplodocus

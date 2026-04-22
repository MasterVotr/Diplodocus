#pragma once

#include <cstdint>

namespace diplodocus {

struct RaytracingStats {
    // Ray tracing statistics
    float frame_time = 0.0;
    float raytracing_time = 0.0;
    int64_t primary_ray_count = 0;
    int64_t secondary_ray_count = 0;
    int64_t shadow_ray_count = 0;

    // Query/Traversal statistics
    int64_t query_count = 0;
    int64_t intersection_count = 0;
    int64_t traversal_count = 0;
};

}  // namespace diplodocus

#pragma once

namespace diplodocus {

struct RaytracingStats {
    // Ray tracing statistics
    float frame_time = 0.0;
    float raytracing_time = 0.0;
    int primary_ray_count = 0;
    int secondary_ray_count = 0;
    int shadow_ray_count = 0;

    // Query/Traversal statistics
    int query_count = 0;
    int intersection_count = 0;
    int traversal_count = 0;
};

}  // namespace diplodocus

#pragma once

#include "gpu/scene/gpu_ray.h"
#include "stats/raytracing_stats.h"

namespace diplodocus::cuda_kernels {

struct GpuRayContext {
    GpuRay ray;
    int pixel_x;
    int pixel_y;
    int depth;
    RaytracingStats& rt_stats;
};

}  // namespace diplodocus::cuda_kernels

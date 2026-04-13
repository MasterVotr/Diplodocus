#pragma once

#include "gpu/scene/gpu_ray.h"

namespace diplodocus::cuda_kernels {

struct GpuRayContext {
    GpuRay ray;
    int pixel_x;
    int pixel_y;
    int depth;
};

}  // namespace diplodocus::cuda_kernels

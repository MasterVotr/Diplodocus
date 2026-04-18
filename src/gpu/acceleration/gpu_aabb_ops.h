#pragma once

#include "gpu/acceleration/gpu_aabb.h"
#include "gpu/cuda_math.h"
#include "gpu/scene/gpu_ray.h"

namespace diplodocus::cuda_kernels {

DI bool IntersectRayAabb(const GpuRay& ray, const GpuAabb& aabb, float& t_min, float& t_max) {
    const float3 inv_dir = Splat(1.0f) / ray.dir;
    const float3 t0 = (aabb.bb_min - ray.origin) * inv_dir;
    const float3 t1 = (aabb.bb_max - ray.origin) * inv_dir;
    const float3 t_smaller = Fmin(t0, t1);
    const float3 t_bigger = Fmax(t0, t1);

    t_min = Fmax(Fmax(t_smaller.x, t_smaller.y), t_smaller.z);
    t_max = Fmin(Fmin(t_bigger.x, t_bigger.y), t_bigger.z);

    return t_max >= t_min;
}

}  // namespace diplodocus::cuda_kernels

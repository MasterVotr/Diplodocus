#pragma once

#include <cmath>

#include "gpu/acceleration/gpu_aabb.h"
#include "gpu/cuda_compat.h"
#include "gpu/cuda_math.h"
#include "gpu/scene/gpu_ray.h"

namespace diplodocus::cuda_kernels {

DI bool IntersectRayAabb(const GpuRay& ray, const GpuAabb& aabb, float& t_min, float& t_max) {
    const float3 inv_dir = Splat(1.0f) / ray.dir;
    const float3 t0 = (aabb.min - ray.origin) * inv_dir;
    const float3 t1 = (aabb.max - ray.origin) * inv_dir;
    const float3 t_smaller = Fmin(t0, t1);
    const float3 t_bigger = Fmax(t0, t1);

    t_min = Fmax(Fmax(t_smaller.x, t_smaller.y), t_smaller.z);
    t_max = Fmin(Fmin(t_bigger.x, t_bigger.y), t_bigger.z);

    return t_max >= t_min;
}

HDI float CalculateAabbSurfaceArea(const GpuAabb& a) {
    float3 size = a.max - a.min;
    return 2 * (size.x * size.y + size.y * size.z + size.z * size.x);
}

HDI GpuAabb MergeAabb(const GpuAabb& a, const GpuAabb& b) {
    return {
        make_float3(fminf(a.min.x, b.min.x), fminf(a.min.y, b.min.y), fminf(a.min.z, b.min.z)),
        make_float3(fmaxf(a.max.x, b.max.x), fmaxf(a.max.y, b.max.y), fmaxf(a.max.z, b.max.z)),
    };
}

struct MergeAabbFunctor {
    HDI GpuAabb operator()(const GpuAabb& a, const GpuAabb& b) const { return MergeAabb(a, b); }
};

}  // namespace diplodocus::cuda_kernels

#pragma once

#include "gpu/cuda_compat.h"
#include "gpu/cuda_math.h"
#include "gpu/scene/gpu_ray.h"

namespace diplodocus::cuda_kernels {

DI float3 RayAt(const GpuRay& ray, float t) { return ray.origin + ray.dir * t; }

DI float RayEpsilon(float3 ray_hit_pos, float t_hit) {
    float p_scale = Fmax(Fmax(fabsf(ray_hit_pos.x), fabsf(ray_hit_pos.y)), Fmax(fabsf(ray_hit_pos.z), 1.0f));
    float t_scale = Fmax(t_hit, 1.0f);

    float p_bias = 1e-6f * p_scale;
    float t_bias = 1e-7f * t_scale;
    float bias = Fmax(p_bias, t_bias);

    return Clamp(bias, 1e-7f, 1e-3f);
}

DI float3 RayOffsetOrigin(float3 origin, float eps, float3 geom_normal, float3 new_dir) {
    float3 offset = Dot(new_dir, geom_normal) > 0.0f ? geom_normal : -geom_normal;
    float3 new_pos = origin + offset * eps;

    return new_pos;
}

}  // namespace diplodocus::cuda_kernels

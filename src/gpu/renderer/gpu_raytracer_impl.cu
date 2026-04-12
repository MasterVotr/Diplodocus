#include "gpu/cuda_math.h"
#include "gpu/renderer/gpu_raytracer_impl.h"
#include "gpu/scene/gpu_triangle_ops.h"

namespace diplodocus::cuda_kernels {

D float3 TraceRay(GpuTraceContext trace_ctx, GpuRayContext ray_ctx) {
    float t_max = ray_ctx.ray.t_max;
    // Intersect scene
    bool hit = false;
    float t_hit = ray_ctx.ray.t_max;
    int tri_hit;
    float b1_hit, b2_hit;

    // TODO: Change to acceleration stucture
    float t, b1, b2;
    for (int tri = 0; tri < trace_ctx.scene.triangle_count; tri++) {
        t = IntersectRayTriangle({trace_ctx.scene.triangle_v0_pos[tri], trace_ctx.scene.triangle_v1_pos[tri],
                                  trace_ctx.scene.triangle_v2_pos[tri]},
                                 ray_ctx.ray, b1, b2, false);
        if (t > kEpsilon && t < t_hit) {
            hit = true;
            t_hit = t;
            tri_hit = tri;
            b1_hit = b1;
            b2_hit = b2;
        }
    }

    // Early return of background color on miss
    if (!hit) return {1.0f, 0.0f, 1.0f};  // TODO: Change to background color

    // LocalIlluminantion
    return Splat(t / t_max);

    // Reflection
    // Refraction

    float3 ray_color = {1.0f, 0.0f, 1.0f};
    return ray_color;
}

}  // namespace diplodocus::cuda_kernels

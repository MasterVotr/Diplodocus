#include <vector_types.h>

#include "gpu/cuda_math.h"
#include "gpu/renderer/gpu_raytracer.h"
#include "gpu/renderer/gpu_renderer_util.h"
#include "gpu/scene/gpu_ray.h"
#include "gpu/scene/gpu_ray_hit.h"
#include "gpu/scene/gpu_ray_ops.h"

namespace diplodocus::cuda_kernels {

D float3 TraceRay(GpuTraceContext trace_ctx, GpuRayContext ray_ctx) {
    // Scene intersection
    GpuRayHit ray_hit;
    bool hit = DummyIntersect(trace_ctx.scene, ray_ctx.ray, ray_hit, false);
    if (!hit) return trace_ctx.render_config.background_color;

    // If light was hit, return its emissive value
    const auto& mat_emission = trace_ctx.scene.mat_emission[ray_hit.material_id];
    if (!AlmostEqual(mat_emission, Splat(0.0f))) return mat_emission;

    // LocalIllumination
    float3 ray_color = Splat(0.0f);
    // ray_color = ray_color + LocalIlluminationPointLights(trace_ctx, ray_ctx, ray_hit);
    ray_color = ray_color + LocalIlluminationAreaLights(trace_ctx, ray_ctx.pixel_x, ray_ctx.pixel_y, ray_hit);

    if (ray_ctx.depth < trace_ctx.render_config.max_depth) {
        // if (ray_ctx.depth > 1) printf("Pixel [%d, %d] at depth %d\n", ray_ctx.pixel_x, ray_ctx.pixel_y, ray_ctx.depth);
        // Reflection
        if (!AlmostEqual(trace_ctx.scene.mat_specular[ray_hit.material_id], Splat(0.0f))) {
            float3 refl_dir = Reflect(ray_ctx.ray, ray_hit);
            float3 refl_origin = RayOffsetOrigin(ray_hit.pos, ray_hit.epsilon, ray_hit.geom_normal, refl_dir);
            GpuRay refl_ray{refl_origin, refl_dir, ray_ctx.ray.t_max};
            GpuRayContext new_ray_ctx = ray_ctx;
            new_ray_ctx.ray = refl_ray;
            new_ray_ctx.depth = ray_ctx.depth + 1;
            ray_color = ray_color + TraceRay(trace_ctx, new_ray_ctx);
        }

        // Refraction
        if (!AlmostEqual(trace_ctx.scene.mat_transmittance[ray_hit.material_id], Splat(0.0f))) {
            float3 refr_dir = Refract(ray_ctx.ray, ray_hit, trace_ctx.scene.mat_ior[ray_hit.material_id],
                                      trace_ctx.scene.mat_r_ior[ray_hit.material_id]);
            float3 refr_origin = RayOffsetOrigin(ray_hit.pos, ray_hit.epsilon, ray_hit.geom_normal, refr_dir);
            GpuRay refr_ray{refr_origin, refr_dir, ray_ctx.ray.t_max};
            GpuRayContext new_ray_ctx = ray_ctx;
            new_ray_ctx.ray = refr_ray;
            new_ray_ctx.depth = ray_ctx.depth + 1;
            ray_color = ray_color + TraceRay(trace_ctx, new_ray_ctx);
        }
    }

    return ray_color;
}

}  // namespace diplodocus::cuda_kernels

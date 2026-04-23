#pragma once

#include "gpu/cuda_compat.h"
#include "gpu/cuda_math.h"
#include "gpu/cuda_utils.h"
#include "gpu/renderer/gpu_ray_context.h"
#include "gpu/renderer/gpu_renderer_util.h"
#include "gpu/renderer/gpu_trace_context.h"
#include "gpu/scene/gpu_ray_hit.h"

namespace diplodocus::cuda_kernels {

template <typename Acceleration>
D float3 TracePath(GpuTraceContext<Acceleration>& trace_ctx, GpuRayContext ray_ctx) {
    ray_ctx.rt_stats.primary_ray_count++;
    float3 radiance = Splat(0.0f);
    float3 throughput = Splat(1.0f);
    const auto& scene = trace_ctx.scene;
    int seed = trace_ctx.render_config.seed;

    // Outer while loop
    int max_depth = trace_ctx.render_config.max_depth;
    // while (!LessAll(throughput, Splat(0.01f))) {
    while (ray_ctx.depth < max_depth) {
        GpuRayHit ray_hit;
        bool hit = trace_ctx.accel.Intersect(ray_ctx.rt_stats, scene, ray_ctx.ray, ray_hit, false);

        // Scene missed -> early exit with background color
        if (!hit) {
            radiance = radiance + throughput * trace_ctx.render_config.background_color;
            break;
        }

        // Emissive triangles hit -> early exit with emission
        float3 ke = scene.mat_emission[ray_hit.material_id];
        if (!AlmostEqual(ke, Splat(0.0f))) {
            radiance = radiance + throughput * ke;
            break;
        }

        // Local illuminations contribution
        float3 pl_contrib = LocalIlluminationPointLights<Acceleration>(ray_ctx.rt_stats, trace_ctx, ray_hit);
        radiance = radiance + throughput * pl_contrib;
        float3 al_contrib = LocalIlluminationAreaLights<Acceleration>(ray_ctx.rt_stats, trace_ctx, ray_ctx, ray_hit);
        radiance = radiance + throughput * al_contrib;

        // Reflection and Refraction
        float3 ks = scene.mat_specular[ray_hit.material_id];
        float3 kt = scene.mat_transmittance[ray_hit.material_id];
        bool has_refl = !AlmostEqual(ks, Splat(0.0f));
        bool has_refr = !AlmostEqual(kt, Splat(0.0f));

        // No secondary rays
        if (!has_refl && !has_refr) break;

        // Has only reflection
        float prob_refl = 1.0f;
        // Has both reflection and refraction
        if (has_refl && has_refr) {
            float fresnel = SchlickFresnel(ray_ctx.ray, ray_hit, scene.mat_ior[ray_hit.material_id]);
            float weight_refl = fresnel * Luminance(ks);
            float weight_refr = (1.0f - fresnel) * Luminance(kt);
            prob_refl = weight_refl / (Fmax(kEpsilon, weight_refl + weight_refr));
        } else if (!has_refl && has_refr) {  // Has only refraction
            prob_refl = 0.0f;
        }

        float r = HashValuesU01(seed, ray_ctx.pixel_x, ray_ctx.pixel_y, ray_ctx.pixel_s, ray_ctx.depth);
        if (r <= prob_refl) {  // Reflect
            float3 refl_dir = Reflect(ray_ctx.ray, ray_hit);
            float3 refl_origin = RayOffsetOrigin(ray_hit.pos, ray_hit.epsilon, ray_hit.geom_normal, refl_dir);
            ray_ctx.ray = {refl_origin, refl_dir, ray_ctx.ray.t_max};
            throughput = throughput * ks / Fmax(kEpsilon, prob_refl);
            ray_ctx.rt_stats.secondary_ray_count++;
        } else {  // Refract
            bool tir = false;
            float3 refr_dir = Refract(ray_ctx.ray, ray_hit, scene.mat_ior[ray_hit.material_id],
                                      scene.mat_r_ior[ray_hit.material_id], tir);
            float3 refr_origin = RayOffsetOrigin(ray_hit.pos, ray_hit.epsilon, ray_hit.geom_normal, refr_dir);
            ray_ctx.ray = {refr_origin, refr_dir, ray_ctx.ray.t_max};
            if (tir) {
                // If TIR happens -> force full reflection
                throughput = throughput * kt;
            } else {
                float prob_refr = 1.0f - prob_refl;
                throughput = throughput * kt / Fmax(kEpsilon, prob_refr);
            }
            ray_ctx.rt_stats.secondary_ray_count++;
        }

        // Increment depth for next bounce
        ray_ctx.depth++;
    }

    return radiance;
}

}  // namespace diplodocus::cuda_kernels

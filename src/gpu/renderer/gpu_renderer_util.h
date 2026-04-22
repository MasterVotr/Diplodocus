#pragma once

#include <vector_types.h>

#include <cstdint>

#include "gpu/cuda_math.h"
#include "gpu/cuda_utils.h"
#include "gpu/renderer/gpu_ray_context.h"
#include "gpu/renderer/gpu_trace_context.h"
#include "gpu/scene/gpu_ray.h"
#include "gpu/scene/gpu_ray_hit.h"
#include "gpu/scene/gpu_ray_ops.h"
#include "gpu/scene/gpu_scene.h"
#include "gpu/scene/gpu_triangle_ops.h"
#include "stats/raytracing_stats.h"

namespace diplodocus::cuda_kernels {

constexpr float kGamma = 2.2f;

DI float RandomAreaLightSample01(uint32_t seed, uint32_t px, uint32_t py, uint32_t ps, uint32_t l, uint32_t ls,
                                 uint32_t d) {
    uint32_t key = seed;
    key = HashCombine(key, px);
    key = HashCombine(key, py);
    key = HashCombine(key, ps);
    key = HashCombine(key, l);
    key = HashCombine(key, ls);
    key = HashCombine(key, d);

    return U01FromU32(HashU32(key));
}

template <typename Acceleration>
DI bool IsShadowed(RaytracingStats& rt_stats, const GpuTraceContext<Acceleration>& trace_ctx, const GpuRayHit& ray_hit,
                   float3 pl_pos) {
    rt_stats.shadow_ray_count++;
    float3 to_light = pl_pos - ray_hit.pos;
    float dist_to_light = Length(to_light);

    // Early exit if on light sruface
    if (dist_to_light < kEpsilon) return false;

    float3 shadow_dir = to_light / dist_to_light;
    float3 shadow_origin = RayOffsetOrigin(ray_hit.pos, ray_hit.epsilon, ray_hit.geom_normal, shadow_dir);
    GpuRay shadow_ray{shadow_origin, shadow_dir, dist_to_light * (1.0f - ray_hit.epsilon)};

    return trace_ctx.accel.IntersectAny(rt_stats, trace_ctx.scene, shadow_ray, false);
}

template <typename Acceleration>
DI float3 LocalIlluminationPointLights(RaytracingStats& rt_stats, const GpuTraceContext<Acceleration>& trace_ctx,
                                       const GpuRayHit& ray_hit) {
    float3 color = Splat(0.0f);  // Black color
    const auto& scene = trace_ctx.scene;

    for (int pl = 0; pl < scene.pl_cnt; pl++) {
        if (IsShadowed<Acceleration>(rt_stats, trace_ctx, ray_hit, scene.pl_pos[pl])) continue;

        // Phong
        // Compute the light direction, view direction and reflection direction
        float3 d_l = Normalize(scene.pl_pos[pl] - ray_hit.pos);
        float3 d_v = Normalize(trace_ctx.cam_pos - ray_hit.pos);
        float3 d_r = Normalize(ray_hit.normal * 2.0f * Dot(ray_hit.normal, d_l) - d_l);

        // Compute ambient, diffuse, specular
        float3 I_a = scene.pl_color[pl] * Splat(0.0f);  // useless Ambient color
        float3 I_d = scene.pl_color[pl] * scene.mat_diffuse[ray_hit.material_id] * Fmax(0.0f, Dot(ray_hit.normal, d_l));
        float3 I_s = scene.pl_color[pl] * scene.mat_specular[ray_hit.material_id] *
                     Pow(Fmax(0.0f, Dot(d_v, d_r)), scene.mat_shininess[ray_hit.material_id]);

        color = color + I_a + I_d + I_s;
    }

    return color;
}

template <typename Acceleration>
DI float3 LocalIlluminationAreaLights(RaytracingStats& rt_stats, const GpuTraceContext<Acceleration>& trace_ctx,
                                      const GpuRayContext& ray_ctx, const GpuRayHit& ray_hit) {
    float3 color = Splat(0.0f);  // Black color
    const auto& scene = trace_ctx.scene;
    int seed = trace_ctx.render_config.seed;
    int al_sample_cnt = trace_ctx.render_config.area_light_sample_cnt;
    int pixel_x = ray_ctx.pixel_x;
    int pixel_y = ray_ctx.pixel_y;
    int pixel_s = ray_ctx.pixel_s;

    for (int al = 0; al < scene.al_cnt; al++) {
        int al_t = scene.al_tri_id[al];
        const auto& al_color = scene.al_color[al];

        // If light hit dirrectly, just recturn light color
        if (al_t == ray_hit.triangle_id) return al_color;

        for (int s = 0; s < al_sample_cnt; s++) {
            // Point light sample
            float r1 = RandomAreaLightSample01(seed, pixel_x, pixel_y, pixel_s, al, s, 0);
            float r2 = RandomAreaLightSample01(seed, pixel_x, pixel_y, pixel_s, al, s, 1);
            float3 pl_pos =
                TriangleSampleSurface(scene.tri_v0_pos[al_t], scene.tri_v1_pos[al_t], scene.tri_v2_pos[al_t], r1, r2);
            if (IsShadowed<Acceleration>(rt_stats, trace_ctx, ray_hit, pl_pos)) continue;

            // Light contribution
            float3 to_light = pl_pos - ray_hit.pos;
            float dist_to_light = Length(to_light);
            float3 d_l = to_light / dist_to_light;
            float w = (scene.al_surface_area[al] * Fmax(0.0f, Dot(scene.tri_geom_norm[al_t], (-d_l))) *
                       Fmax(0.0f, Dot(ray_hit.normal, d_l))) /
                      ((al_sample_cnt * dist_to_light * dist_to_light) + kEpsilon);
            float3 pl_color = al_color * w;

            // Phong
            // Compute the light direction, view direction and reflection direction
            float3 d_v = Normalize(trace_ctx.cam_pos - ray_hit.pos);
            float3 d_r = Normalize(ray_hit.normal * 2.0f * Dot(ray_hit.normal, d_l) - d_l);

            // Compute ambient, diffuse, specular
            float3 I_a = pl_color * Splat(0.0f);  // useless Ambient color
            float3 I_d = pl_color * scene.mat_diffuse[ray_hit.material_id] * Fmax(0.0f, Dot(ray_hit.normal, d_l));
            float3 I_s = pl_color * scene.mat_specular[ray_hit.material_id] *
                         Pow(Fmax(0.0f, Dot(d_v, d_r)), scene.mat_shininess[ray_hit.material_id]);

            color = color + I_a + I_d + I_s;
        }
    }

    return color;
}

DI float3 Reflect(const GpuRay& ray, const GpuRayHit& ray_hit) {
    float3 v = -ray.dir;
    float3 refl_dir = -v + 2.0f * Dot(v, ray_hit.normal) * ray_hit.normal;
    return refl_dir;
}

DI float3 Refract(const GpuRay& ray, const GpuRayHit& ray_hit, float ior, float r_ior) {
    float3 v = -ray.dir;
    float3 n = ray_hit.normal;
    float eta = ior;

    float cos_i = Dot(v, ray_hit.normal);

    if (cos_i < 0.0f) {
        // Flip if inside the object
        eta = r_ior;
        cos_i = -cos_i;
        n = -n;
    }

    float sin2_i = Fmax(0.0f, 1.0f - (cos_i * cos_i));
    float sin2_t = sin2_i / (eta * eta);

    // TIR
    if (sin2_t >= 1.0f) return Reflect(ray, ray_hit);

    float cos_t = Sqrt(1.0f - sin2_t);

    float3 refr_dir = -v / eta + (cos_i / eta - cos_t) * n;
    return refr_dir;
}

DI float SchlickFresnel(const GpuRay& ray, const GpuRayHit& ray_hit, float ior) {
    float cos_i = Fmax(0.0f, Dot(-ray.dir, ray_hit.normal));
    float eta_i = 1.0f;
    float eta_t = ior;
    float r0 = (eta_i - eta_t) / (eta_i + eta_t);
    r0 = r0 * r0;
    float fresnel = r0 + (1.0f - r0) * Pow(1.0f - cos_i, 5.0f);

    return fresnel;
}

// Calculates the brightness of color (based on ITU-R Recommendation 709)
DI float Luminance(float3 color) { return 0.2126f * color.x + 0.7152 * color.y + 0.0722f * color.z; }

}  // namespace diplodocus::cuda_kernels

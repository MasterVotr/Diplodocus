#include <vector_types.h>

#include "gpu/cuda_math.h"
#include "gpu/renderer/gpu_raytracer_impl.h"
#include "gpu/scene/gpu_ray.h"
#include "gpu/scene/gpu_ray_hit.h"
#include "gpu/scene/gpu_ray_ops.h"
#include "gpu/scene/gpu_triangle_ops.h"

namespace diplodocus::cuda_kernels {

constexpr int kSampleCount = 16;                         // TODO: Get from config
constexpr int kMaxDepth = 8;                             // TODO: Get from config
constexpr float3 kBackgroundColor = {0.0f, 0.0f, 0.0f};  // TODO: Get from config

DI float RandomAreaLightSample01(uint32_t seed, uint32_t px, uint32_t py, uint32_t light_id, uint32_t sample_idx,
                                 uint32_t dimension) {
    uint32_t key = 0;
    key ^= seed * 0x9e3779b1U;
    key ^= px * 0x165667b1U;
    key ^= py * 0xd3a2646cU;
    key ^= light_id * 0x85ebca6bU;
    key ^= sample_idx * 0xc2b2ae35U;
    key ^= dimension * 0x27d4eb2fU;

    return U01FromU32(HashU32(key));
}

DI bool DummyIntersect(const GpuSceneView& scene, const GpuRayContext& ray_ctx, GpuRayHit& ray_hit,
                       bool backface_culling) {
    bool hit = false;
    float t_hit = ray_ctx.ray.t_max;
    int tri_hit;
    float b1_hit, b2_hit;

    const auto& tri_v0_pos = scene.tri_v0_pos;
    const auto& tri_v1_pos = scene.tri_v1_pos;
    const auto& tri_v2_pos = scene.tri_v2_pos;

    // Intersect with scene
    float t, b1, b2;
    for (int tri = 0; tri < scene.tri_cnt; tri++) {
        t = IntersectRayTriangle(tri_v0_pos[tri], tri_v1_pos[tri], tri_v2_pos[tri], ray_ctx.ray, b1, b2, false);
        if (t > kEpsilon && t < t_hit) {
            hit = true;
            t_hit = t;
            tri_hit = tri;
            b1_hit = b1;
            b2_hit = b2;
        }
    }

    // Early return if miss
    if (!hit) return false;

    // Calculate RayHit info
    ray_hit.pos = RayAt(ray_ctx.ray, t_hit);
    ray_hit.normal = scene.tri_goem_norm[tri_hit];
    ray_hit.geom_normal = scene.tri_goem_norm[tri_hit];
    ray_hit.b0 = 1.0f - b1_hit - b2_hit;
    ray_hit.b1 = b1_hit;
    ray_hit.b2 = b2_hit;
    ray_hit.t = t_hit;
    ray_hit.triangle_id = tri_hit;
    ray_hit.material_id = scene.tri_mat_id[tri_hit];
    ray_hit.epsilon = RayEpsilon(ray_hit.pos, ray_hit.t);
    if (scene.tri_has_vn[tri_hit]) {
        ray_hit.normal = Normalize(scene.tri_v0_norm[tri_hit] * ray_hit.b0 + scene.tri_v1_norm[tri_hit] * ray_hit.b1 +
                                   scene.tri_v2_norm[tri_hit] * ray_hit.b2);
    }

    return true;
}

DI bool DummyIntersectAny(const GpuSceneView& scene, const GpuRay& ray, bool backface_culling) {
    bool hit = false;
    float t_hit = ray.t_max;
    int tri_hit;
    float b1_hit, b2_hit;

    const auto& tri_v0_pos = scene.tri_v0_pos;
    const auto& tri_v1_pos = scene.tri_v1_pos;
    const auto& tri_v2_pos = scene.tri_v2_pos;

    // Intersect with scene
    float t, b1, b2;
    for (int tri = 0; tri < scene.tri_cnt; tri++) {
        t = IntersectRayTriangle(tri_v0_pos[tri], tri_v1_pos[tri], tri_v2_pos[tri], ray, b1, b2, false);
        if (t > kEpsilon && t < t_hit) {
            return true;
        }
    }

    return false;
}

DI bool IsShadowed(const GpuTraceContext& trace_ctx, const GpuRayHit& ray_hit, float3 pl_pos) {
    float3 to_light = pl_pos - ray_hit.pos;
    float dist_to_light = Length(to_light);

    // Early exit if on light sruface
    if (dist_to_light < kEpsilon) return false;

    float3 shadow_dir = to_light / dist_to_light;
    float3 shadow_origin = RayOffsetOrigin(ray_hit.pos, ray_hit.epsilon, ray_hit.geom_normal, shadow_dir);
    GpuRay shadow_ray{shadow_origin, shadow_dir, dist_to_light * (1.0f - ray_hit.epsilon)};

    return DummyIntersectAny(trace_ctx.scene, shadow_ray, false);
}

DI float3 LocalIlluminationPointLights(const GpuTraceContext& trace_ctx, const GpuRayContext& ray_ctx,
                                       const GpuRayHit& ray_hit) {
    float3 color = Splat(0.0f);  // Black color
    const auto& scene = trace_ctx.scene;

    for (int pl = 0; pl < scene.pl_cnt; pl++) {
        if (IsShadowed(trace_ctx, ray_hit, scene.pl_pos[pl])) continue;

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

DI float3 LocalIlluminationAreaLights(const GpuTraceContext& trace_ctx, const GpuRayContext& ray_ctx,
                                      const GpuRayHit& ray_hit) {
    float3 color = Splat(0.0f);  // Black color
    const auto& scene = trace_ctx.scene;

    for (int al = 0; al < scene.al_cnt; al++) {
        int al_t = scene.al_tri_id[al];
        const auto& al_color = scene.al_color[al];

        // If light hit dirrectly, just recturn light color
        if (al_t == ray_hit.triangle_id) return al_color;

        for (int s = 0; s < kSampleCount; s++) {
            // Point light sample
            float r1 = RandomAreaLightSample01(42, ray_ctx.pixel_x, ray_ctx.pixel_y, al, s, 0);
            float r2 = RandomAreaLightSample01(42, ray_ctx.pixel_x, ray_ctx.pixel_y, al, s, 1);
            float3 pl_pos =
                TriangleSampleSurface(scene.tri_v0_pos[al_t], scene.tri_v1_pos[al_t], scene.tri_v2_pos[al_t], r1, r2);
            if (IsShadowed(trace_ctx, ray_hit, pl_pos)) continue;

            // Light contribution
            float3 to_light = pl_pos - ray_hit.pos;
            float dist_to_light = Length(to_light);
            float3 d_l = to_light / dist_to_light;
            float w = (scene.al_surface_area[al] * Fmax(0.0f, Dot(scene.tri_goem_norm[al_t], (-d_l))) *
                       Fmax(0.0f, Dot(ray_hit.normal, d_l))) /
                      ((kSampleCount * dist_to_light * dist_to_light) + kEpsilon);
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

D float3 TraceRay(GpuTraceContext trace_ctx, GpuRayContext ray_ctx) {
    // Scene intersection
    GpuRayHit ray_hit;
    bool hit = DummyIntersect(trace_ctx.scene, ray_ctx, ray_hit, false);
    if (!hit) return kBackgroundColor;

    // If light was hit, return its emissive value
    const auto& mat_emission = trace_ctx.scene.mat_emission[ray_hit.material_id];
    if (!AlmostEqual(mat_emission, Splat(0.0f))) return mat_emission;

    // LocalIllumination
    float3 ray_color = Splat(0.0f);
    // ray_color = ray_color + LocalIlluminationPointLights(trace_ctx, ray_ctx, ray_hit);
    ray_color = ray_color + LocalIlluminationAreaLights(trace_ctx, ray_ctx, ray_hit);

    if (ray_ctx.depth < kMaxDepth) {
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

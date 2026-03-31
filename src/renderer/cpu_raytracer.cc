#include "renderer/cpu_raytracer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "config/render_config.h"
#include "framebuffer/framebuffer.h"
#include "renderer/renderer.h"
#include "scene/light.h"
#include "scene/material.h"
#include "scene/ray.h"
#include "scene/ray_hit.h"
#include "scene/triangle.h"
#include "util/colors.h"
#include "util/logger.h"
#include "util/timer.h"
#include "util/util.h"
#include "util/vec3.h"

namespace diplodocus {

namespace {

inline float RandomAreaLightSample01(uint32_t seed, uint32_t px, uint32_t py, uint32_t light_id, uint32_t sample_idx,
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

inline float RayEpsilon(const Vec3& ray_hit_pos, float t_hit) {
    float p_scale = std::max({std::fabs(ray_hit_pos.x), std::fabs(ray_hit_pos.y), std::fabs(ray_hit_pos.z), 1.0f});
    float t_scale = std::max(t_hit, 1.0f);

    float p_bias = 1e-6f * p_scale;
    float t_bias = 1e-7f * t_scale;
    float bias = std::max(p_bias, t_bias);

    return std::clamp(bias, 1e-7f, 1e-3f);
}

inline Vec3 OffsetRayOrigin(const Vec3& pos, float eps, const Vec3& geom_normal, const Vec3& new_dir) {
    Vec3 offset = Dot(new_dir, geom_normal) > 0.0f ? geom_normal : -geom_normal;
    Vec3 new_pos = pos + offset * eps;

    return new_pos;
}

}  // namespace

RenderResult CpuRaytracer::StartRender(const RenderConfig& render_config, const Scene& scene, Framebuffer& framebuffer,
                                       Stats& stats) {
    Logger::info("CPU Renderer: Rendering...");
    framebuffer.Resize(render_config.width, render_config.height);
    framebuffer.Clear();

    // TODO: Setup ADS using acceleration_config and CreateAccelerationStucture factory

    Timer rt_time;

    const int w = framebuffer.GetWidth();
    const int h = framebuffer.GetHeight();
    const Vec3& cam_dir = scene.GetCamera().dir;
    const Vec3& cam_up = scene.GetCamera().up;
    const float cam_fov = scene.GetCamera().fov;

    const Vec3 cam_right = Normalize(Cross(cam_dir, cam_up));
    const float gw = 2.0 * std::tan(cam_fov / 2.0f);
    const float gh = gw * (static_cast<float>(h) / static_cast<float>(w));
    const Vec3 qw = cam_right * (gw / (static_cast<float>(w - 1)));
    const Vec3 qh = -cam_up * (gh / (static_cast<float>(h - 1)));
    const Vec3 p00 = cam_dir - cam_right * (gw / 2) + cam_up * (gh / 2);

    TraceContext trace_ctx{render_config, scene, framebuffer, stats, p00, qw, qh};
    for (int y = 0; y < h; y++) {
        progress_ = static_cast<float>(y) / static_cast<float>(h - 1);
        for (int x = 0; x < w; x++) {
            PixelContext pixel_context{x, y};
            Vec3 pixel_color = TracePixel(trace_ctx, pixel_context);
            framebuffer.SetPixel(x, y, pixel_color);
        }
        if (y % ((h - 1) / 25) == 0) {
            Logger::debug("Render progress: {:.2f} %", progress_ * 100.0f);
        }
    }

    stats.rt_stats.raytracing_time = rt_time.elapsed_s();
    Logger::info("CPU Renderer: Rendering done");

    return RenderResult::kDone;
}

void CpuRaytracer::Reset() {
    cancelled_ = false;
    progress_ = 0.0f;
}

Vec3 CpuRaytracer::TracePixel(const TraceContext& trace_ctx, PixelContext pixel_ctx) {
    Vec3 r_d = Normalize(trace_ctx.p00 + (trace_ctx.qw * pixel_ctx.x) + (trace_ctx.qh * pixel_ctx.y));
    Ray ray = {trace_ctx.scene.GetCamera().pos, r_d, kEpsilon, trace_ctx.scene.GetCamera().far};
    Vec3 ray_color = TraceRay(trace_ctx, pixel_ctx, ray, 0);
    trace_ctx.stats.rt_stats.primary_ray_count += 1;

    return ray_color;
}

Vec3 CpuRaytracer::TraceRay(const TraceContext& trace_ctx, const PixelContext& pixel_ctx, const Ray& ray, int depth) {
    bool hit = false;
    float t_hit = ray.t_max;
    int tri_hit;
    float b1_hit, b2_hit;
    auto triangles = trace_ctx.scene.Triangles();
    size_t tri = 0;

    // Intersection with the scene  TODO: Change to an ADS
    float b1, b2, t;
    for (; tri < triangles.size(); tri++) {
        t = triangles[tri].IntersectRay(ray, b1, b2, trace_ctx.render_config.backface_culling);
        if (t > ray.t_min && t != kInfinity && t < t_hit) {
            hit = true;
            t_hit = t;
            tri_hit = tri;
            b1_hit = b1;
            b2_hit = b2;
        }
    }

    // Background color for misses
    if (!hit) return trace_ctx.render_config.background_color;

    // Calculate RayHit info
    const Triangle& triangle_hit = triangles[tri_hit];
    const Material& material = trace_ctx.scene.Materials()[triangle_hit.material_id];
    RayHit ray_hit;
    ray_hit.t = t_hit;
    ray_hit.triangle_id = tri_hit;
    ray_hit.pos = ray.At(t_hit);
    ray_hit.b0 = 1.0f - b1_hit - b2_hit;
    ray_hit.b1 = b1_hit;
    ray_hit.b2 = b2_hit;
    ray_hit.epsilon = RayEpsilon(ray_hit.pos, ray_hit.t);
    ray_hit.geom_normal = triangle_hit.geom_normal;
    ray_hit.normal = triangle_hit.geom_normal;
    if (triangle_hit.has_vertex_normals) {
        ray_hit.normal = Normalize(triangle_hit.v0.normal * ray_hit.b0 + triangle_hit.v1.normal * ray_hit.b1 +
                                   triangle_hit.v2.normal * ray_hit.b2);
    }

    // If light was hit, return its emissive value
    if (material.emission != Vec3(0.0f)) return material.emission;

    // Phong/Cook-Torrens
    Vec3 ray_color = LocalIllumination(trace_ctx, pixel_ctx, ray_hit);

    if (depth < trace_ctx.render_config.max_depth) {
        // Reflection
        if (material.specular != Vec3(0.0f)) {
            trace_ctx.stats.rt_stats.secondary_ray_count += 1;
            Ray reflection_ray = ReflectionRay(ray, ray_hit);

            // Debug self intersection
            t = triangle_hit.IntersectRay(reflection_ray, b1, b2);
            if (t > reflection_ray.t_min && t < reflection_ray.t_max) {
                Logger::debug("Reflected ray self intersects - need to move origin more or increase t_min");
            }

            Vec3 I_R = TraceRay(trace_ctx, pixel_ctx, reflection_ray, depth + 1);
            ray_color += I_R * material.specular;
        }

        // Refraction
        if (material.transmittance != Vec3(0.0f)) {
            trace_ctx.stats.rt_stats.secondary_ray_count += 1;
            Ray refraction_ray = RefractionRay(ray, ray_hit, material.ior, material.r_ior);

            // Debug self intersection
            t = triangle_hit.IntersectRay(refraction_ray, b1, b2);
            if (t > refraction_ray.t_min && t < refraction_ray.t_max) {
                Logger::debug("Refracted ray self intersects - need to move origin more or increase t_min");
            }

            Vec3 I_T = TraceRay(trace_ctx, pixel_ctx, refraction_ray, depth + 1);
            ray_color += I_T * material.transmittance;
        }
    }

    return ray_color;
}

Vec3 CpuRaytracer::LocalIllumination(const TraceContext& trace_ctx, const PixelContext& pixel_ctx,
                                     const RayHit& ray_hit) {
    Vec3 color = color::kBlack;
    // Point lights
    for (const auto& point_light : trace_ctx.scene.PointLights()) {
        // If shadowed discard
        if (IsShadowed(trace_ctx, ray_hit, point_light)) continue;

        // Light attenuation TODO
        // float d = Length(point_light.pos - ray_hit.pos);
        // Vec3 I_l = (point_light.color * point_light.power) / ((d * d) + kEpsilon);
        //
        // color += Phong(trace_ctx, ray_hit, {point_light.pos, I_l, point_light.power});
        color += Phong(trace_ctx, ray_hit, point_light);
    }

    // Area Lights
    int sample_cnt = trace_ctx.render_config.area_light_sample_cnt;
    for (size_t al = 0; al < trace_ctx.scene.AreaLights().size(); al++) {
        const auto& area_light = trace_ctx.scene.AreaLights()[al];

        // Check if triangle hit is the area light
        if (area_light.triangle_id == ray_hit.triangle_id) return area_light.color;

        const Triangle& light_triangle = trace_ctx.scene.Triangles()[area_light.triangle_id];
        for (int s = 0; s < sample_cnt; s++) {
            // Sample light position
            PointLight light_sample;
            float r1 = RandomAreaLightSample01(42, pixel_ctx.x, pixel_ctx.y, al, s, 0);
            float r2 = RandomAreaLightSample01(42, pixel_ctx.x, pixel_ctx.y, al, s, 1);
            light_sample.pos = light_triangle.SampleSurface(r1, r2);

            // If shadowed discard
            if (IsShadowed(trace_ctx, ray_hit, light_sample)) continue;

            // Sample light color
            Vec3 to_light = light_sample.pos - ray_hit.pos;
            float d = Length(to_light);
            Vec3 d_l = to_light / d;
            float w = (area_light.surface_area * std::max(0.0f, Dot(light_triangle.geom_normal, (-d_l))) *
                       std::max(0.0f, Dot(ray_hit.normal, d_l))) /
                      ((sample_cnt * d * d) + kEpsilon);
            light_sample.color = area_light.color * w;

            // Sample light contribution
            color += Phong(trace_ctx, ray_hit, light_sample);
        }
    }

    return color;
}

bool CpuRaytracer::IsShadowed(const TraceContext& trace_ctx, const RayHit& ray_hit, const PointLight& light) {
    trace_ctx.stats.rt_stats.shadow_ray_count += 1;
    Vec3 to_light = light.pos - ray_hit.pos;
    float dist_to_light = Length(to_light);

    // Early exit if on the surface of light
    if (dist_to_light < kEpsilon) return false;

    // Create shadow ray
    Vec3 shadow_dir = to_light / dist_to_light;
    Vec3 shadow_origin = OffsetRayOrigin(ray_hit.pos, ray_hit.epsilon, ray_hit.geom_normal, shadow_dir);
    Ray shadow_ray = {shadow_origin, shadow_dir, 0.0f, dist_to_light * (1 - ray_hit.epsilon)};

    // Intersection with the scene  TODO: Change to an ADS
    float b1, b2, t;
    auto triangles = trace_ctx.scene.Triangles();
    for (size_t tri = 0; tri < triangles.size(); tri++) {
        t = triangles[tri].IntersectRay(shadow_ray, b1, b2, trace_ctx.render_config.backface_culling);
        if (t > shadow_ray.t_min && t <= shadow_ray.t_max) {
            if ((int)tri == ray_hit.triangle_id) {
                Logger::error("Shadow ray self intersects - need to move origin more or increase t_min");
                continue;
            }
            return true;
        }
    }

    return false;
}

Vec3 CpuRaytracer::Phong(const TraceContext& trace_ctx, const RayHit& ray_hit, const PointLight& light) {
    const Material& material =
        trace_ctx.scene.Materials()[trace_ctx.scene.Triangles()[ray_hit.triangle_id].material_id];

    // Compute the light direction, view direction and reflection direction
    Vec3 d_l = Normalize(light.pos - ray_hit.pos);
    Vec3 d_v = Normalize(trace_ctx.scene.GetCamera().pos - ray_hit.pos);
    Vec3 d_r = Normalize(ray_hit.normal * 2.0f * Dot(ray_hit.normal, d_l) - d_l);

    // Compute ambient, diffuse, specular, reflection and reflaction components
    auto I_a = light.color * color::kBlack;  // useless Ambient color
    auto I_d = light.color * material.diffuse * std::max(0.0f, Dot(ray_hit.normal, d_l));
    auto I_s = light.color * material.specular * std::pow(std::max(0.0f, Dot(d_v, d_r)), material.shininess);

    return I_a + I_d + I_s;  // + I_e;
}

Ray CpuRaytracer::ReflectionRay(const Ray& ray, const RayHit& ray_hit) {
    Vec3 d_v = -ray.dir;
    Vec3 refl_dir = -d_v + 2.0f * Dot(d_v, ray_hit.normal) * ray_hit.normal;
    Vec3 refl_origin = OffsetRayOrigin(ray_hit.pos, ray_hit.epsilon, ray_hit.geom_normal, refl_dir);
    return {refl_origin, refl_dir, ray_hit.epsilon, ray.t_max};
}

Ray CpuRaytracer::RefractionRay(const Ray& ray, const RayHit& ray_hit, float ior, float r_ior) {
    Vec3 v = -ray.dir;
    Vec3 n = ray_hit.normal;
    float eta = ior;

    float cos_i = Dot(v, ray_hit.normal);

    if (cos_i < 0.0f) {
        // Flip if inside the object
        eta = r_ior;
        cos_i = -cos_i;
        n = -n;
    }

    float sin2_i = std::max(0.0f, 1.0f - (cos_i * cos_i));
    float sin2_t = sin2_i / (eta * eta);

    // TIR
    if (sin2_t >= 1.0f) return ReflectionRay(ray, ray_hit);

    float cos_t = std::sqrt(1.0f - sin2_t);

    Vec3 refr_dir = -v / eta + (cos_i / eta - cos_t) * n;
    Vec3 refr_origin = OffsetRayOrigin(ray_hit.pos, ray_hit.epsilon, ray_hit.geom_normal, refr_dir);
    return {refr_origin, refr_dir, ray_hit.epsilon, ray.t_max};
}

}  // namespace diplodocus

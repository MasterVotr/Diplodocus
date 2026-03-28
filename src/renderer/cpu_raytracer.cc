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

}  // namespace

RenderResult CpuRaytracer::StartRender(const RenderConfig& render_config, const Scene& scene, Framebuffer& framebuffer,
                                       Stats& stats) {
    Logger::debug("CPU Renderer: Starting rendering");
    framebuffer.Resize(render_config.width, render_config.height);

    // TODO: Setup ADS using acceleration_config and CreateAccelerationStucture factory

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
    }

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
    for (; tri < triangles.size(); tri++) {
        float b1, b2;
        float t = triangles[tri].IntersectRay(ray, b1, b2, trace_ctx.render_config.backface_culling);
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
    ray_hit.normal = triangle_hit.geom_normal;
    // ray_hit.normal = Normalize(triangle_hit.v0.normal * ray_hit.b0 + triangle_hit.v1.normal * ray_hit.b1 +
    //                            triangle_hit.v2.normal * ray_hit.b2);

    // Phong/Cook-Torrens
    Vec3 ray_color = LocalIllumination(trace_ctx, pixel_ctx, ray_hit);

    if (depth < trace_ctx.render_config.max_depth) {
        // Reflection
        // TODO

        // Refraction
        // TODO
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
    // int sample_cnt = trace_ctx.render_config.area_light_sample_cnt;
    // for (size_t al = 0; al < trace_ctx.scene.AreaLights().size(); al++) {
    //     const auto& area_light = trace_ctx.scene.AreaLights()[al];
    //     const Triangle& light_triangle = trace_ctx.scene.Triangles()[area_light.triangle_id];
    //     for (int s = 0; s < sample_cnt; s++) {
    //         // Sample light position
    //         PointLight light_sample;
    //         float r1 = RandomAreaLightSample01(42, pixel_ctx.x, pixel_ctx.y, al, s, 0);
    //         float r2 = RandomAreaLightSample01(42, pixel_ctx.x, pixel_ctx.y, al, s, 1);
    //         light_sample.pos = light_triangle.SampleSurface(r1, r2);
    //
    //         // If shadowed discard
    //         if (IsShadowed(trace_ctx, ray_hit, light_sample)) continue;
    //
    //         // Sample light color
    //         Vec3 from_light_to_hit_pos = light_sample.pos - ray_hit.pos;
    //         float d = Length(from_light_to_hit_pos);
    //         Vec3 d_l = {from_light_to_hit_pos.x / d, from_light_to_hit_pos.y / d, from_light_to_hit_pos.z / d};
    //         float w = (area_light.surface_area * std::max(0.0f, Dot(light_triangle.normal, (-d_l)))) /
    //                   ((sample_cnt * d * d) + kEpsilon);
    //         light_sample.color = area_light.color * w;
    //
    //         // Sample light contribution
    //         color += Phong(trace_ctx, ray_hit, light_sample);
    //     }
    // }

    return color;
}

bool CpuRaytracer::IsShadowed(const TraceContext& trace_ctx, const RayHit& ray_hit, const PointLight& light) {
    Vec3 to_light = light.pos - ray_hit.pos;
    float dist_to_light = Length(to_light);

    // Early exit if on the surface of light
    if (dist_to_light < kEpsilon) return false;

    Vec3 shadow_ray_dir = to_light / dist_to_light;
    // Move the shadow ray origin to avoid selfintersection
    // TODO: Possibly change to -normal based on Dot with shadow ray dir to correctly shadow refracted rays
    Vec3 shadow_ray_origin = ray_hit.pos + trace_ctx.scene.Triangles()[ray_hit.triangle_id].geom_normal * kEpsilon;
    Ray shadow_ray = {shadow_ray_origin, shadow_ray_dir, kEpsilon, dist_to_light - kEpsilon};
    auto triangles = trace_ctx.scene.Triangles();

    // Intersection with the scene  TODO: Change to an ADS
    for (size_t tri = 0; tri < triangles.size(); tri++) {
        float b1, b2;
        float t = triangles[tri].IntersectRay(shadow_ray, b1, b2, false);
        if (t > shadow_ray.t_min && t != kInfinity && t < shadow_ray.t_max) {
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
    // auto I_e = material.emission;

    return I_a + I_d + I_s;  // + I_e;
}

Ray CpuRaytracer::ReflectionRay(const RayHit* rayhit) {
    // TODO
}

Ray CpuRaytracer::ReflectionRay(const RayHit* rayhit, float ior) {
    // TODO
}

}  // namespace diplodocus

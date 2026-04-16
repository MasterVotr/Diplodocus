#pragma once

#include <atomic>

#include "acceleration/acceleration_structure.h"
#include "config/acceleration_structure_config.h"
#include "config/render_config.h"
#include "framebuffer/framebuffer.h"
#include "renderer/renderer.h"
#include "scene/light.h"
#include "scene/ray.h"
#include "scene/ray_hit.h"
#include "scene/scene.h"
#include "stats/stats.h"
#include "util/vec3.h"

namespace diplodocus {

class CpuRaytracer : public Renderer {
   public:
    RenderResult StartRender(const RenderConfig& render_config, const AccelerationStructureConfig& acceleration_config,
                             const Scene& scene, Framebuffer& framebuffer, Stats& stats) override;
    void Reset() override;
    void Cancel() override { cancelled_ = true; }
    float GetProgress() const override { return progress_; }
    inline const char* GetName() const override { return "CPU Raytracer"; }

   private:
    struct TraceContext {
        const RenderConfig& render_config;
        std::unique_ptr<AccelerationStructure> accel_struct;
        const Scene& scene;
        Framebuffer& framebuffer;
        Stats& stats;

        Vec3 p00;
        Vec3 qw;
        Vec3 qh;
    };

    struct PixelContext {
        int x, y;
    };

    std::atomic<bool> cancelled_ = false;
    std::atomic<float> progress_ = 0.0f;

    static Vec3 TracePixel(const TraceContext& trace_ctx, PixelContext pixel_ctx);
    static Vec3 TraceRay(const TraceContext& trace_ctx, const PixelContext& pixel_ctx, const Ray& ray, int depth);
    static Vec3 LocalIlluminationAreaLights(const TraceContext& trace_ctx, const PixelContext& pixel_ctx,
                                            const RayHit& ray_hit);
    static Vec3 LocalIlluminationPointLights(const TraceContext& trace_ctx, const RayHit& ray_hit);
    static bool IsShadowed(const TraceContext& trace_ctx, const RayHit& ray_hit, const PointLight& light);
    static Vec3 Phong(const TraceContext& trace_ctx, const RayHit& ray_hit, const PointLight& light);
    static Ray ReflectionRay(const Ray& ray, const RayHit& ray_hit);
    static Ray RefractionRay(const Ray& ray, const RayHit& ray_hit, float ior, float r_ior);
};

}  // namespace diplodocus

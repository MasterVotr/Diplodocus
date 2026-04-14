#include "renderer/gpu_raytracer.h"

#include "gpu/framebuffer/gpu_framebuffer.h"
#include "gpu/renderer/gpu_renderer_api.h"
#include "gpu/scene/gpu_scene.h"
#include "renderer/renderer.h"
#include "util/logger.h"
#include "util/timer.h"

namespace diplodocus {

RenderResult GpuRaytracer::StartRender(const RenderConfig& render_config,
                                       const AccelerationStructureConfig& acceleration_config, const Scene& scene,
                                       Framebuffer& framebuffer, Stats& stats) {
    // cuda_kernels::HelloCunda();

    Logger::info("GPU Renderer: Rendering...");
    Timer rt_time;

    // Setup ray calculation variables
    const int w = render_config.width;
    const int h = render_config.height;
    const Vec3& cam_dir = scene.GetCamera().dir;
    const Vec3& cam_up = scene.GetCamera().up;
    const float cam_fov = scene.GetCamera().fov;
    const Vec3& cam_pos = scene.GetCamera().pos;

    const Vec3 cam_right = Normalize(Cross(cam_dir, cam_up));
    const float gw = 2.0 * std::tan(cam_fov / 2.0f);
    const float gh = gw * (static_cast<float>(h) / static_cast<float>(w));
    const Vec3 qw = cam_right * (gw / (static_cast<float>(w - 1)));
    const Vec3 qh = -cam_up * (gh / (static_cast<float>(h - 1)));
    const Vec3 p00 = cam_dir - cam_right * (gw / 2) + cam_up * (gh / 2);

    // Transfer scene, framebuffer to GPU
    cuda_kernels::GpuFramebuffer gpu_framebuffer;
    gpu_framebuffer.Resize(render_config.width, render_config.height);
    cuda_kernels::GpuScene gpu_scene(scene);
    // TODO: transfer config, stats

    // Create trace context
    cuda_kernels::GpuTraceContext gpu_trace_ctx{
        gpu_scene.GetSceneView(), gpu_framebuffer.GetFramebufferView(), {p00.x, p00.y, p00.z}, {qw.x, qw.y, qw.z},
        {qh.x, qh.y, qh.z},       {cam_pos.x, cam_pos.y, cam_pos.z},    scene.GetCamera().far,
    };

    // Launch Kernels
    cuda_kernels::LaunchClearFramebufferKernel(
        gpu_framebuffer.GetFramebufferView(),
        {render_config.background_color.x, render_config.background_color.y, render_config.background_color.z});

    // cuda_kernels::LaunchRaytracingStackKernel(gpu_trace_ctx);
    cuda_kernels::LaunchRaytracingBounceKernel(gpu_trace_ctx);

    // Download framebuffer from device to host
    gpu_framebuffer.Download(framebuffer);

    stats.rt_stats.raytracing_time = rt_time.elapsed_s();

    return RenderResult::kDone;
}

void GpuRaytracer::Reset() {}

}  // namespace diplodocus

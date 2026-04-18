#include "renderer/gpu_renderer.h"

#include <cmath>

#include "config/acceleration_structure_config.h"
#include "framebuffer/framebuffer.h"
#include "gpu/acceleration/gpu_acceleration_structure.h"
#include "gpu/acceleration/gpu_bvh_build_api.h"
#include "gpu/acceleration/gpu_intersection.h"
#include "gpu/config/gpu_acceleration_structure_config.h"
#include "gpu/config/gpu_config_bridge.h"
#include "gpu/config/gpu_render_config.h"
#include "gpu/framebuffer/gpu_framebuffer.h"
#include "gpu/renderer/gpu_renderer_api.h"
#include "gpu/renderer/gpu_trace_context.h"
#include "gpu/scene/gpu_scene.h"
#include "renderer/renderer.h"
#include "stats/stats.h"
#include "util/logger.h"
#include "util/timer.h"

namespace diplodocus {

namespace {

template <cuda_kernels::BoundingVolumeType BV>
RenderResult StartRenderImpl(Stats& stats, cuda_kernels::GpuSceneView gpu_scene,
                             const cuda_kernels::GpuBuildParams& gpu_build_params,
                             const cuda_kernels::GpuRenderConfig& gpu_render_config,
                             cuda_kernels::GpuFramebufferView gpu_framebuffer, const Vec3& p00, const Vec3& qw,
                             const Vec3& qh, const Vec3& cam_pos, float cam_far) {
    // Build BVH
    Timer build_t;
    int tri_count = gpu_scene.tri_cnt;
    int node_count = 2 * tri_count - 1;
    cuda_kernels::GpuBvh<BV> gpu_bvh(node_count, tri_count);
    cuda_kernels::LaunchBuildBvhKernels<BV>(gpu_build_params, gpu_bvh.GetView());
    stats.accel_stats.build_time = build_t.elapsed_ms();

    // Create trace context
    cuda_kernels::BvhAcceleration<BV> gpu_accel{gpu_bvh.GetView()};
    cuda_kernels::GpuTraceContext<cuda_kernels::BvhAcceleration<BV>> gpu_trace_ctx{
        gpu_render_config,
        gpu_scene,
        gpu_accel,
        gpu_framebuffer,
        {p00.x, p00.y, p00.z},
        {qw.x, qw.y, qw.z},
        {qh.x, qh.y, qh.z},
        {cam_pos.x, cam_pos.y, cam_pos.z},
        cam_far,
    };

    // Render using BVH
    Timer rt_time;
    cuda_kernels::LaunchPathtracingKernel<cuda_kernels::BvhAcceleration<BV>>(gpu_trace_ctx);
    stats.rt_stats.raytracing_time = rt_time.elapsed_s();

    return RenderResult::kDone;
}

}  // namespace

RenderResult GpuRenderer::StartRender(const RenderConfig& render_config,
                                      const AccelerationStructureConfig& acceleration_config, const Scene& scene,
                                      Framebuffer& framebuffer, Stats& stats) {
    Logger::info("GPU Renderer: Rendering...");

    // Setup ray calculation variables
    const int w = render_config.width;
    const int h = render_config.height;
    const Vec3& cam_dir = scene.GetCamera().dir;
    const Vec3& cam_up = scene.GetCamera().up;
    const float cam_fov = scene.GetCamera().fov;
    const Vec3& cam_pos = scene.GetCamera().pos;
    const float cam_far = scene.GetCamera().far;

    const Vec3 cam_right = Normalize(Cross(cam_dir, cam_up));
    const float gw = 2.0 * std::tan(cam_fov / 2.0f);
    const float gh = gw * (static_cast<float>(h) / static_cast<float>(w));
    const Vec3 qw = cam_right * (gw / (static_cast<float>(w - 1)));
    const Vec3 qh = -cam_up * (gh / (static_cast<float>(h - 1)));
    const Vec3 p00 = cam_dir - cam_right * (gw / 2) + cam_up * (gh / 2);

    // Create a framebuffer on the gpu
    cuda_kernels::GpuFramebuffer gpu_framebuffer;
    gpu_framebuffer.Resize(render_config.width, render_config.height);
    cuda_kernels::LaunchClearFramebufferKernel(
        gpu_framebuffer.GetFramebufferView(),
        {render_config.background_color.x, render_config.background_color.y, render_config.background_color.z});

    // Transfer scene, framebuffer to GPU  TODO: transfer stats
    cuda_kernels::GpuScene gpu_scene(scene);
    cuda_kernels::GpuRenderConfig gpu_render_config = cuda_kernels::BridgeRenderConfig(render_config);
    cuda_kernels::GpuAccelerationStructureConfig gpu_accel_config =
        cuda_kernels::BridgeAccelerationConfig(acceleration_config);

    // Create build params
    cuda_kernels::GpuBuildParams gpu_build_params{
        gpu_accel_config,
        gpu_scene.GetView(),
    };

    // Launch Build and Render
    RenderResult render_result{RenderResult::kError};
    switch (acceleration_config.acceleration_structure_type) {
        case AccelerationStructureType::kPloc:
        case AccelerationStructureType::kPlocEmc: {
            render_result = StartRenderImpl<cuda_kernels::BoundingVolumeType::kAabb>(
                stats, gpu_scene.GetView(), gpu_build_params, gpu_render_config, gpu_framebuffer.GetFramebufferView(),
                p00, qw, qh, cam_pos, cam_far);
            break;
        }
        case AccelerationStructureType::kPlocSobb:
        case AccelerationStructureType::kPlocEmcSobb: {
            render_result = StartRenderImpl<cuda_kernels::BoundingVolumeType::kSobb>(
                stats, gpu_scene.GetView(), gpu_build_params, gpu_render_config, gpu_framebuffer.GetFramebufferView(),
                p00, qw, qh, cam_pos, cam_far);
            break;
        }
        case AccelerationStructureType::kDummy: {
            // Create trace context
            cuda_kernels::NoAcceleration gpu_noaccel;
            cuda_kernels::GpuTraceContext<cuda_kernels::NoAcceleration> gpu_trace_ctx{
                gpu_render_config,
                gpu_scene.GetView(),
                gpu_noaccel,
                gpu_framebuffer.GetFramebufferView(),
                {p00.x, p00.y, p00.z},
                {qw.x, qw.y, qw.z},
                {qh.x, qh.y, qh.z},
                {cam_pos.x, cam_pos.y, cam_pos.z},
                cam_far,
            };

            // Render
            Timer rt_time;
            // cuda_kernels::LaunchRaytracingStackKernel(gpu_trace_ctx);
            cuda_kernels::LaunchPathtracingKernel(gpu_trace_ctx);
            stats.rt_stats.raytracing_time = rt_time.elapsed_s();
            render_result = RenderResult::kDone;

            break;
        }
        default: {
            Logger::error("GpuRenderer: Chosen acceleration structure type is not available on the GPU");
            render_result = RenderResult::kError;
        }
    }

    // Download framebuffer
    gpu_framebuffer.Download(framebuffer);

    return render_result;
}

void GpuRenderer::Reset() {}

}  // namespace diplodocus

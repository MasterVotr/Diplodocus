#pragma once

#include <vector_types.h>

#include "gpu/acceleration/gpu_acceleration_structure.h"
#include "gpu/config/gpu_acceleration_structure_config.h"
#include "gpu/config/gpu_render_config.h"
#include "gpu/framebuffer/gpu_framebuffer.h"
#include "gpu/scene/gpu_scene.h"

namespace diplodocus::cuda_kernels {

template <typename Acceleration>
struct GpuTraceContext {
    GpuRenderConfig render_config;
    GpuSceneView scene;
    Acceleration accel;
    GpuFramebufferView framebuffer;
    // GpuStats stats;

    float3 p00;
    float3 qw;
    float3 qh;
    float3 cam_pos;
    float cam_far;
};

}  // namespace diplodocus::cuda_kernels

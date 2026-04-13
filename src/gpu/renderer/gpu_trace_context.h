#pragma once

#include <vector_types.h>

#include "gpu/framebuffer/gpu_framebuffer.h"
#include "gpu/scene/gpu_scene.h"

namespace diplodocus::cuda_kernels {

struct GpuTraceContext {
    // GpuConfig config;
    GpuSceneView scene;
    GpuFramebufferView framebuffer;
    // GpuStats stats;

    float3 p00;
    float3 qw;
    float3 qh;
    float3 cam_pos;
    float cam_far;
};

}  // namespace diplodocus::cuda_kernels

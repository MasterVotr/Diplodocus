#pragma once

#include "gpu/config/gpu_acceleration_structure_config.h"
#include "gpu/framebuffer/gpu_framebuffer.h"
#include "gpu/renderer/gpu_trace_context.h"

namespace diplodocus::cuda_kernels {

void HelloCunda();
void LaunchClearFramebufferKernel(const GpuFramebufferView& framebuffer, float3 color);
// void LaunchRaytracingKernel(const GpuTraceContext& trace_ctx);
// void LaunchPathtracingKernel(const GpuTraceContext& trace_ctx);

template <typename Acceleration>
void LaunchPathtracingKernel(const GpuTraceContext<Acceleration>& trace_ctx);

}  // namespace diplodocus::cuda_kernels

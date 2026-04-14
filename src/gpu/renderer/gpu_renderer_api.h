#pragma once

#include "gpu/framebuffer/gpu_framebuffer.h"
#include "gpu/renderer/gpu_trace_context.h"

namespace diplodocus::cuda_kernels {

void HelloCunda();
void LaunchClearFramebufferKernel(const GpuFramebufferView& framebuffer, float3 color);
void LaunchRaytracingStackKernel(const GpuTraceContext& trace_ctx);
void LaunchRaytracingBounceKernel(const GpuTraceContext& trace_ctx);

}  // namespace diplodocus::cuda_kernels

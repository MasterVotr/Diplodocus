#pragma once

#include "gpu/framebuffer/gpu_framebuffer.h"
#include "gpu/renderer/gpu_trace_context.h"
#include "stats/raytracing_stats.h"

namespace diplodocus::cuda_kernels {

void HelloCunda();
void LaunchClearFramebufferKernel(const GpuFramebufferView& framebuffer, float3 color);
// void LaunchRaytracingKernel(const GpuTraceContext& trace_ctx);
// void LaunchPathtracingKernel(const GpuTraceContext& trace_ctx);

template <typename Acceleration>
void LaunchPathtracingKernel(const GpuTraceContext<Acceleration>& trace_ctx, RaytracingStats& rt_stats);

}  // namespace diplodocus::cuda_kernels

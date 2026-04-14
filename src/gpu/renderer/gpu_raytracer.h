#pragma once

#include "gpu/cuda_compat.h"
#include "gpu/renderer/gpu_ray_context.h"
#include "gpu/renderer/gpu_trace_context.h"

namespace diplodocus::cuda_kernels {

D float3 TraceRay(GpuTraceContext trace_ctx, GpuRayContext ray_ctx);

}  // namespace diplodocus::cuda_kernels

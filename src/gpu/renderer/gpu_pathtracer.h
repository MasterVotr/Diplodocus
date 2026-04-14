#pragma once

#include "gpu/cuda_compat.h"
#include "gpu/renderer/gpu_ray_context.h"
#include "gpu/renderer/gpu_trace_context.h"

namespace diplodocus::cuda_kernels {

D float3 TracePath(GpuTraceContext trace_ctx, GpuRayContext bounce_state);

}  // namespace diplodocus::cuda_kernels

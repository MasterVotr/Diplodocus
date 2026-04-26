#pragma once

#include <vector_types.h>

namespace diplodocus::cuda_kernels {

struct GpuAabb {
    float3 min;
    float3 max;
};

static_assert(sizeof(GpuAabb) == 24, "GpuAabb: unexpected sizeof()");

}  // namespace diplodocus::cuda_kernels

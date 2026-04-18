#pragma once

#include <vector_types.h>

namespace diplodocus::cuda_kernels {

struct GpuAabb {
    float3 bb_min;
    float3 bb_max;
};

}  // namespace diplodocus::cuda_kernels

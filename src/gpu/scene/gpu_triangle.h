#pragma once

#include <vector_types.h>

namespace diplodocus::cuda_kernels {

struct GpuTriangle {
    float3 v0_pos;
    float3 v1_pos;
    float3 v2_pos;
};

}  // namespace diplodocus::cuda_kernels

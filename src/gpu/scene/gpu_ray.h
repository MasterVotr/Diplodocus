#pragma once

#include <vector_types.h>

namespace diplodocus::cuda_kernels {

struct GpuRay {
    float3 origin;
    float3 dir;
    float t_max;
};

}  // namespace diplodocus::cuda_kernels

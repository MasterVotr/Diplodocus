#pragma once

#include "gpu/cuda_math.h"

namespace diplodocus::cuda_kernels {

struct GpuRayHit {
    float3 pos;
    float3 normal;
    float3 geom_normal;
    float b0, b1, b2;
    float t;
    int triangle_id{0};
    int material_id{-1};
    float epsilon{kEpsilon};
};

}  // namespace diplodocus::cuda_kernels

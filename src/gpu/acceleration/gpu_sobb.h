#pragma once

#include <cstdint>

namespace diplodocus::cuda_kernels {

struct GpuDop {
    float2 slab[16];  // x = slab min, y = slab max
};

struct GpuSobb {
    float3 b_mins;
    float3 b_maxs;
    uint32_t n_ids;
};

static_assert(sizeof(GpuDop) == 2 * 4 * 16, "GpuSobb: unexpected sizeof()");
static_assert(sizeof(GpuSobb) == 2 * 3 * 4 + 4, "GpuSobb: unexpected sizeof()");

}  // namespace diplodocus::cuda_kernels

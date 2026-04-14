#pragma once

#include <vector_types.h>

namespace diplodocus::cuda_kernels {

struct GpuRenderConfig {
    float3 background_color = {0.0f, 0.0f, 0.0f};
    bool backface_culling = false;
    int max_depth = 8;
    int area_light_sample_cnt = 2;
    int pixel_sample_cnt = 32;
    int seed = 42;
};

}  // namespace diplodocus::cuda_kernels

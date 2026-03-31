#pragma once

#include "util/colors.h"
#include "util/vec3.h"

namespace diplodocus {

struct RenderConfig {
    int width = 800;
    int height = 600;
    Vec3 background_color = color::kBlack;
    bool backface_culling = false;
    int max_depth = 4;
    int area_light_sample_cnt = 8;
};

}  // namespace diplodocus

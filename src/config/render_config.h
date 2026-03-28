#pragma once

#include "util/colors.h"
#include "util/vec3.h"

namespace diplodocus {

struct RenderConfig {
    int width = 1280;
    int height = 720;
    Vec3 background_color = color::kDarkGray;
    bool backface_culling = false;
    int max_depth = 0;
    int area_light_sample_cnt = 4;
};

}  // namespace diplodocus

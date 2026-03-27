#pragma once

// #include <vector>

#include "util/vec3.h"

namespace diplodocus {

struct PointLight {
    Vec3 pos;
    Vec3 color;
};

struct AreaLight {
    Vec3 color;
    int triangle_id;
    float surface_area;
};

}  // namespace diplodocus

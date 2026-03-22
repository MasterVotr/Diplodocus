#pragma once

#include "util/vec3.h"

namespace diplodocus {

struct PointLight {
    Vec3 pos;
    Vec3 color;
};

struct AreaLight {
    int triangle_id;
    Vec3 color;
};

}  // namespace diplodocus

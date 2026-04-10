#pragma once

#include "util/util.h"
#include "util/vec3.h"

namespace diplodocus {

struct RayHit {
    Vec3 pos;
    Vec3 normal;
    Vec3 geom_normal;
    float b0, b1, b2;
    float t;
    int triangle_id = -1;
    int material_id = -1;
    float epsilon = kEpsilon;
};

}  // namespace diplodocus

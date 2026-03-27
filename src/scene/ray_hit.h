#pragma once

#include "util/vec3.h"

namespace diplodocus {

struct RayHit {
    Vec3 pos;
    Vec3 normal;
    float b0, b1, b2;
    float t;
    int triangle_id = -1;
};

}  // namespace diplodocus

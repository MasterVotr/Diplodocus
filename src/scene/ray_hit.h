#pragma once

#include <cstdint>

#include "util/vec3.h"

namespace diplodocus {

struct RayHit {
    int triangle_id;
    Vec3 pos;
    Vec3 normal;
    float t, u, v;
};

}  // namespace diplodocus

#pragma once

#include "util/vec3.h"

namespace diplodocus {

struct Ray {
    Vec3 origin;
    Vec3 direction;
    float t_min, t_max;

    inline Vec3 At(float t) const { return origin + direction * t; }
};

}  // namespace diplodocus

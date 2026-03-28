#pragma once

#include "util/util.h"
#include "util/vec3.h"

namespace diplodocus {

struct Camera {
    Vec3 pos;
    Vec3 dir;
    Vec3 up;
    float fov;
    float near = kEpsilon;
    float far;
};

}  // namespace diplodocus

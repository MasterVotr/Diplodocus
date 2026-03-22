#pragma once

#include "util/vec3.h"

namespace diplodocus {

struct Camera {
    Vec3 pos;
    Vec3 dir;
    Vec3 up;
    float fov;
    float near;
    float far;
};

}  // namespace diplodocus

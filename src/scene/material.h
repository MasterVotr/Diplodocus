#pragma once

#include "util/vec3.h"

namespace diplodocus {

struct Material {
    Vec3 ambient;
    Vec3 diffuse;
    Vec3 specular;
    Vec3 transmittance;
    Vec3 emission;
    float shininess;
    float ior;
    float dissolve;
};

}  // namespace diplodocus

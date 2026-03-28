#pragma once

#include "util/vec3.h"

namespace diplodocus {

struct Material {
    std::string name;
    Vec3 ambient;
    Vec3 diffuse;
    Vec3 specular;
    Vec3 transmittance;
    Vec3 emission;
    float shininess;
    float ior;
    float r_ior;
    float dissolve;
};

}  // namespace diplodocus

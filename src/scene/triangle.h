#pragma once

#include "scene/vertex.h"
#include "util/vec3.h"

namespace diplodocus {

struct Ray;
struct RayHit;

struct Triangle {
    Vertex v0, v1, v2;
    Vec3 normal;
    uint32_t material_id;

    RayHit IntersectRay(Ray& ray, bool backface_culling);
};

}  // namespace diplodocus

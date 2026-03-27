#pragma once

#include "scene/vertex.h"
#include "util/vec3.h"

namespace diplodocus {

struct Ray;

struct Triangle {
    Vertex v0, v1, v2;
    Vec3 geom_normal;
    uint32_t material_id;

    float IntersectRay(const Ray& ray, float& b1, float& b2, bool backface_culling = true, float eps = kEpsilon) const;
    Vec3 SampleSurface(float r1, float r2) const;
};

inline void RecomputeTriangleGeometricNormal(Triangle& t) {
    const auto e1 = t.v1.pos - t.v0.pos;
    const auto e2 = t.v2.pos - t.v0.pos;
    t.geom_normal = Normalize(Cross(e1, e2));
}

}  // namespace diplodocus

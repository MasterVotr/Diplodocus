#pragma once

#include "scene/vertex.h"
#include "util/util.h"
#include "util/vec3.h"

namespace diplodocus {

struct Ray;

struct Triangle {
    Vertex v0, v1, v2;
    Vec3 geom_normal;
    int32_t material_id;
    bool has_vertex_normals;

    float IntersectRay(const Ray& ray, float& b1, float& b2, bool backface_culling = true, float eps = kEpsilon) const;
    Vec3 SampleSurface(float r1, float r2) const;
};

inline void RecomputeTriangleGeometricNormal(Triangle& t) {
    const Vec3 e1 = t.v1.pos - t.v0.pos;
    const Vec3 e2 = t.v2.pos - t.v0.pos;
    t.geom_normal = Normalize(Cross(e1, e2));
}

inline float CalculateTriangleSurfaceArea(const Triangle& t) {
    const Vec3 u = t.v1.pos - t.v0.pos;
    const Vec3 v = t.v2.pos - t.v0.pos;
    const Vec3 c = Cross(u, v);
    const float c_magnitude = Length(c);
    return 0.5 * c_magnitude;
}

}  // namespace diplodocus

#pragma once

#include "acceleration/aabb.h"
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

inline Vec3 CalculateTriangleCentroid(const Triangle& t) { return (t.v0.pos + t.v1.pos + t.v2.pos) / 3.0f; }

inline AABB CalculateTriangleAabb(const Triangle& t) {
    Vec3 min = {std::min({t.v0.pos.x, t.v1.pos.x, t.v2.pos.x}), std::min({t.v0.pos.y, t.v1.pos.y, t.v2.pos.y}),
                std::min({t.v0.pos.z, t.v1.pos.z, t.v2.pos.z})};
    Vec3 max = {std::max({t.v0.pos.x, t.v1.pos.x, t.v2.pos.x}), std::max({t.v0.pos.y, t.v1.pos.y, t.v2.pos.y}),
                std::max({t.v0.pos.z, t.v1.pos.z, t.v2.pos.z})};
    return AABB(min, max);
}

}  // namespace diplodocus

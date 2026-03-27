#include "scene/triangle.h"

#include <cstdlib>

#include "scene/ray.h"
#include "util/util.h"

namespace diplodocus {

float Triangle::IntersectRay(const Ray& ray, float& b1, float& b2, bool backface_culling, float eps) const {
    const Vec3& a = v0.pos;
    const Vec3& b = v1.pos;
    const Vec3& c = v2.pos;
    const Vec3 e1(b - a), e2(c - a);
    const Vec3 pvec = Cross(ray.dir, e2);
    const float det = Dot(e1, pvec);

    if (backface_culling) {
        if (det < eps)  // ray is parallel to triangle
            return kInfinity;
    } else {
        if (fabs(det) < eps)  // ray is parallel to triangle
            return kInfinity;
    }

    const float invDet = 1.0f / det;

    // Compute first barycentric coordinate
    const Vec3 tvec = ray.origin - a;
    b1 = Dot(tvec, pvec) * invDet;

    if (b1 < 0.0f || b1 > 1.0f) return kInfinity;

    // Compute second barycentric coordinate
    const Vec3 qvec = Cross(tvec, e1);
    b2 = Dot(ray.dir, qvec) * invDet;

    if (b2 < 0.0f || b1 + b2 > 1.0f) return kInfinity;

    // Compute t to intersection point
    const float t = Dot(e2, qvec) * invDet;
    return t;
}

Vec3 Triangle::SampleSurface(float r1, float r2) const {
    float u = r1 + r2 > 1.0f ? 1.0f - r1 : r1;
    float v = r1 + r2 > 1.0f ? 1.0f - r2 : r2;
    const auto& a = v0.pos;
    const auto& b = v1.pos;
    const auto& c = v2.pos;

    return Vec3(a + (b - a) * u + (c - a) * v);
}

}  // namespace diplodocus

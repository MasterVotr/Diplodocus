#pragma once

#include <vector_functions.h>

#include "gpu/cuda_compat.h"
#include "gpu/cuda_math.h"
#include "gpu/scene/gpu_ray.h"

namespace diplodocus::cuda_kernels {

DI float IntersectRayTriangle(const float3& v0, const float3& v1, const float3& v2, const GpuRay& ray, float& b1,
                              float& b2, bool backface_culling = false, float eps = kEpsilon) {
    const float3 e1(v1 - v0), e2(v2 - v0);
    const float3 pvec = Cross(ray.dir, e2);
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
    const float3 tvec = ray.origin - v0;
    b1 = Dot(tvec, pvec) * invDet;

    if (b1 < 0.0f || b1 > 1.0f) return kInfinity;

    // Compute second barycentric coordinate
    const float3 qvec = Cross(tvec, e1);
    b2 = Dot(ray.dir, qvec) * invDet;

    if (b2 < 0.0f || b1 + b2 > 1.0f) return kInfinity;

    // Compute t to intersection point
    const float t = Dot(e2, qvec) * invDet;
    return t;
}

DI float3 TriangleSampleSurface(const float3& v0, const float3& v1, const float3& v2, float r1, float r2) {
    float u = r1 + r2 > 1.0f ? 1.0f - r1 : r1;
    float v = r1 + r2 > 1.0f ? 1.0f - r2 : r2;
    const auto& a = v0;
    const auto& b = v1;
    const auto& c = v2;

    return a + (b - a) * u + (c - a) * v;
}

}  // namespace diplodocus::cuda_kernels

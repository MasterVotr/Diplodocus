#pragma once

#include <vector_types.h>

#include "gpu/cuda_compat.h"
#include "gpu/cuda_math.h"
#include "gpu/scene/gpu_ray.h"
#include "gpu/scene/gpu_triangle.h"

namespace diplodocus::cuda_kernels {

DI float IntersectRayTriangle(const GpuTriangle& triangle, const GpuRay& ray, float& b1, float& b2,
                              bool backface_culling = false, float eps = kEpsilon) {
    const float3& a = triangle.v0_pos;
    const float3& b = triangle.v1_pos;
    const float3& c = triangle.v2_pos;
    const float3 e1(b - a), e2(c - a);
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
    const float3 tvec = ray.origin - a;
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

}  // namespace diplodocus::cuda_kernels

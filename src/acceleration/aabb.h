#pragma once

#include "scene/ray.h"
#include "util/util.h"
#include "util/vec3.h"

namespace diplodocus {

struct AABB {
    Vec3 min;
    Vec3 max;
    Vec3 size;

    AABB() : min(kInfinity, kInfinity, kInfinity), max(-kInfinity, -kInfinity, -kInfinity) {}
    AABB(const Vec3& min, const Vec3& max) : min(min), max(max), size(Abs(max - min)) {}

    inline float Volumne() const { return size.x * size.y * size.z; }
    inline Vec3 Center() const { return min + (size / 2.0); }
    inline float SurfaceArea() const { return 2.0f * (size.x * size.y + size.y * size.z + size.x * size.z); }
    inline void Expand(const AABB& other) {
        min.x = std::min(min.x, other.min.x);
        min.y = std::min(min.y, other.min.y);
        min.z = std::min(min.z, other.min.z);
        max.x = std::max(max.x, other.max.x);
        max.y = std::max(max.y, other.max.y);
        max.z = std::max(max.z, other.max.z);
        size = max - min;
    }
    inline void Expand(const Vec3& point) {
        min.x = std::min(min.x, point.x);
        min.y = std::min(min.y, point.y);
        min.z = std::min(min.z, point.z);
        max.x = std::max(max.x, point.x);
        max.y = std::max(max.y, point.y);
        max.z = std::max(max.z, point.z);
        size = max - min;
    }
};

inline bool IntersectRayAabb(const Ray& ray, const AABB& aabb, float& t_min, float& t_max) {
    t_min = -kInfinity;
    t_max = kInfinity;

    for (int i = 0; i < 3; ++i) {
        float inv_d = 1.0f / ray.dir[i];
        float t0 = (aabb.min[i] - ray.origin[i]) * inv_d;
        float t1 = (aabb.max[i] - ray.origin[i]) * inv_d;
        if (inv_d < 0.0f) {
            std::swap(t0, t1);
        }
        t_min = std::max(t_min, t0);
        t_max = std::min(t_max, t1);
    }
    return t_max >= t_min;
}

}  // namespace diplodocus

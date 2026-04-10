#pragma once

#include "util/vec3.h"

namespace diplodocus {

struct Ray {
    Vec3 origin;
    Vec3 dir;
    float t_max;

    inline Vec3 At(float t) const { return origin + dir * t; }
};

inline float RayEpsilon(const Vec3& ray_hit_pos, float t_hit) {
    float p_scale = std::max({std::fabs(ray_hit_pos.x), std::fabs(ray_hit_pos.y), std::fabs(ray_hit_pos.z), 1.0f});
    float t_scale = std::max(t_hit, 1.0f);

    float p_bias = 1e-6f * p_scale;
    float t_bias = 1e-7f * t_scale;
    float bias = std::max(p_bias, t_bias);

    return std::clamp(bias, 1e-7f, 1e-3f);
}

}  // namespace diplodocus

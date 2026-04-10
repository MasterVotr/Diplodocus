#include "acceleration/dummy_acceleration_structure.h"

#include "scene/ray.h"
#include "util/logger.h"
#include "util/timer.h"

namespace diplodocus {

void DummyAccelerationStucture::Build(const AccelerationStructureConfig& acceleration_structure_config, Stats& stats,
                                      std::span<const Triangle> triangles) {
    Logger::info("Building dummy acceleration stucture...");
    (void)acceleration_structure_config;
    Timer build_t;

    triangles_ = triangles;

    stats.accel_stats.build_time = build_t.elapsed_ms();
    stats.accel_stats.memory_consumption = sizeof(triangles_);
}

bool DummyAccelerationStucture::Intersect(Stats& stats, const Ray& ray, RayHit& ray_hit, bool backface_culling) const {
    stats.accel_stats.query_count += 1;
    bool hit = false;
    float t_hit = ray.t_max;
    int tri_hit;
    float b1_hit, b2_hit;
    size_t tri = 0;

    // Intersect with scene
    float b1, b2, t;
    for (; tri < triangles_.size(); tri++) {
        t = triangles_[tri].IntersectRay(ray, b1, b2, backface_culling);
        stats.accel_stats.intersection_count += 1;
        if (t > kEpsilon && t < t_hit) {
            hit = true;
            t_hit = t;
            tri_hit = tri;
            b1_hit = b1;
            b2_hit = b2;
        }
    }

    // Return if miss
    if (!hit) return false;

    // Calculate RayHit info
    const Triangle& triangle_hit = triangles_[tri_hit];
    ray_hit.t = t_hit;
    ray_hit.triangle_id = tri_hit;
    ray_hit.material_id = triangle_hit.material_id;
    ray_hit.pos = ray.At(t_hit);
    ray_hit.b0 = 1.0f - b1_hit - b2_hit;
    ray_hit.b1 = b1_hit;
    ray_hit.b2 = b2_hit;
    ray_hit.epsilon = RayEpsilon(ray_hit.pos, ray_hit.t);
    ray_hit.geom_normal = triangle_hit.geom_normal;
    ray_hit.normal = triangle_hit.geom_normal;
    if (triangle_hit.has_vertex_normals) {
        ray_hit.normal = Normalize(triangle_hit.v0.normal * ray_hit.b0 + triangle_hit.v1.normal * ray_hit.b1 +
                                   triangle_hit.v2.normal * ray_hit.b2);
    }

    return true;
}

bool DummyAccelerationStucture::IntersectAny(Stats& stats, const Ray& ray, bool backface_culling) const {
    stats.accel_stats.query_count += 1;

    float b1, b2, t;
    for (size_t tri = 0; tri < triangles_.size(); tri++) {
        t = triangles_[tri].IntersectRay(ray, b1, b2, backface_culling);
        if (t > kEpsilon && t <= ray.t_max) {
            return true;
        }
    }

    return false;
}

}  // namespace diplodocus

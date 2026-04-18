#include "gpu/acceleration/gpu_acceleration_structure.h"
#include "gpu/acceleration/gpu_intersection.h"
#include "gpu/config/gpu_acceleration_structure_config.h"
#include "gpu/scene/gpu_ray.h"
#include "gpu/scene/gpu_ray_hit.h"
#include "gpu/scene/gpu_ray_ops.h"
#include "gpu/scene/gpu_scene.h"
#include "gpu/scene/gpu_triangle_ops.h"

namespace diplodocus::cuda_kernels {

D bool NoAcceleration::Intersect(const GpuSceneView& scene, const GpuRay& ray, GpuRayHit& ray_hit,
                                 bool backface_culling) const {
    bool hit = false;
    float t_hit = ray.t_max;
    int tri_hit;
    float b1_hit, b2_hit;

    const auto& tri_v0_pos = scene.tri_v0_pos;
    const auto& tri_v1_pos = scene.tri_v1_pos;
    const auto& tri_v2_pos = scene.tri_v2_pos;

    // Intersect with scene
    float t, b1, b2;
    for (int tri = 0; tri < scene.tri_cnt; tri++) {
        t = IntersectRayTriangle(tri_v0_pos[tri], tri_v1_pos[tri], tri_v2_pos[tri], ray, b1, b2, backface_culling);
        if (t > kEpsilon && t < t_hit) {
            hit = true;
            t_hit = t;
            tri_hit = tri;
            b1_hit = b1;
            b2_hit = b2;
        }
    }

    // Early return if miss
    if (!hit) return false;

    // Calculate RayHit info
    ray_hit.pos = RayAt(ray, t_hit);
    ray_hit.normal = scene.tri_goem_norm[tri_hit];
    ray_hit.geom_normal = scene.tri_goem_norm[tri_hit];
    ray_hit.b0 = 1.0f - b1_hit - b2_hit;
    ray_hit.b1 = b1_hit;
    ray_hit.b2 = b2_hit;
    ray_hit.t = t_hit;
    ray_hit.triangle_id = tri_hit;
    ray_hit.material_id = scene.tri_mat_id[tri_hit];
    ray_hit.epsilon = RayEpsilon(ray_hit.pos, ray_hit.t);
    if (scene.tri_has_vn[tri_hit]) {
        ray_hit.normal = Normalize(scene.tri_v0_norm[tri_hit] * ray_hit.b0 + scene.tri_v1_norm[tri_hit] * ray_hit.b1 +
                                   scene.tri_v2_norm[tri_hit] * ray_hit.b2);
    }

    return true;
}

D bool NoAcceleration::IntersectAny(const GpuSceneView& scene, const GpuRay& ray, bool backface_culling) const {
    const auto& tri_v0_pos = scene.tri_v0_pos;
    const auto& tri_v1_pos = scene.tri_v1_pos;
    const auto& tri_v2_pos = scene.tri_v2_pos;

    // Intersect with scene
    float t, b1, b2;
    for (int tri = 0; tri < scene.tri_cnt; tri++) {
        t = IntersectRayTriangle(tri_v0_pos[tri], tri_v1_pos[tri], tri_v2_pos[tri], ray, b1, b2, backface_culling);
        if (t > kEpsilon && t < ray.t_max) {
            return true;
        }
    }

    return false;
}

template <>
D bool BvhAcceleration<BoundingVolumeType::kAabb>::Intersect(const GpuSceneView& scene, const GpuRay& ray,
                                                             GpuRayHit& ray_hit, bool backface_culling) const {
    printf("BvhAcceleration::kAabb: Intersect() not implemented yet!\n");
    return false;
}

template <>
D bool BvhAcceleration<BoundingVolumeType::kAabb>::IntersectAny(const GpuSceneView& scene, const GpuRay& ray,
                                                                bool backface_culling) const {
    printf("BvhAcceleration::kAabb: IntersectAny() not implemented yet!\n");
    return false;
}

template <>
D bool BvhAcceleration<BoundingVolumeType::kSobb>::Intersect(const GpuSceneView& scene, const GpuRay& ray,
                                                             GpuRayHit& ray_hit, bool backface_culling) const {
    printf("BvhAcceleration::kSobb: Intersect() not implemented yet!\n");
    return false;
}

template <>
D bool BvhAcceleration<BoundingVolumeType::kSobb>::IntersectAny(const GpuSceneView& scene, const GpuRay& ray,
                                                                bool backface_culling) const {
    printf("BvhAcceleration::kSobb: IntersectAny() not implemented yet!\n");
    return false;
}

}  // namespace diplodocus::cuda_kernels

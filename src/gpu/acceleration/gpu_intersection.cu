#include <cassert>
#include <cstdint>

#include "gpu/acceleration/gpu_aabb_ops.h"
#include "gpu/acceleration/gpu_acceleration_structure.h"
#include "gpu/acceleration/gpu_intersection.h"
#include "gpu/config/gpu_acceleration_structure_config.h"
#include "gpu/cuda_math.h"
#include "gpu/scene/gpu_ray.h"
#include "gpu/scene/gpu_ray_hit.h"
#include "gpu/scene/gpu_ray_ops.h"
#include "gpu/scene/gpu_scene.h"
#include "gpu/scene/gpu_triangle_ops.h"
#include "stats/raytracing_stats.h"

namespace diplodocus::cuda_kernels {

D bool NoAcceleration::Intersect(RaytracingStats& rt_stats, const GpuSceneView& scene, const GpuRay& ray,
                                 GpuRayHit& ray_hit, bool backface_culling) const {
    rt_stats.query_count++;
    rt_stats.intersection_count += scene.tri_cnt;
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
    ray_hit.normal = scene.tri_geom_norm[tri_hit];
    ray_hit.geom_normal = scene.tri_geom_norm[tri_hit];
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

D bool NoAcceleration::IntersectAny(RaytracingStats& rt_stats, const GpuSceneView& scene, const GpuRay& ray,
                                    bool backface_culling) const {
    rt_stats.query_count++;
    const auto& tri_v0_pos = scene.tri_v0_pos;
    const auto& tri_v1_pos = scene.tri_v1_pos;
    const auto& tri_v2_pos = scene.tri_v2_pos;

    // Intersect with scene
    float t, b1, b2;
    for (int tri = 0; tri < scene.tri_cnt; tri++) {
        rt_stats.intersection_count++;
        t = IntersectRayTriangle(tri_v0_pos[tri], tri_v1_pos[tri], tri_v2_pos[tri], ray, b1, b2, backface_culling);
        if (t > kEpsilon && t < ray.t_max) {
            return true;
        }
    }

    return false;
}

template <>
D bool BvhAcceleration<BoundingVolumeType::kAabb>::Intersect(RaytracingStats& rt_stats, const GpuSceneView& scene,
                                                             const GpuRay& ray, GpuRayHit& ray_hit,
                                                             bool backface_culling) const {
    rt_stats.query_count++;
    bool hit = false;
    float t_hit = ray.t_max;
    int tri_hit = -1;
    float b1_hit, b2_hit;

    int32_t stack[64];
    int32_t stack_top = 0;
    stack[stack_top] = *accel.root;

    while (stack_top != -1) {
        const GpuBvhNode<BoundingVolumeType::kAabb>& node = accel.nodes[stack[stack_top]];
        stack_top--;

        // Leaf node
        if (node.is_leaf) {
            rt_stats.intersection_count++;
            int32_t tri_idx = accel.tri_idxs[node.left];
            float b1, b2;
            float t = IntersectRayTriangle(scene.tri_v0_pos[tri_idx], scene.tri_v1_pos[tri_idx],
                                           scene.tri_v2_pos[tri_idx], ray, b1, b2, backface_culling);
            if (t > kEpsilon && t < t_hit) {
                hit = true;
                t_hit = t;
                tri_hit = tri_idx;
                b1_hit = b1;
                b2_hit = b2;
            }
            continue;
        }

        // Internal node
        int32_t left_child_idx = node.left;
        const GpuBvhNode<BoundingVolumeType::kAabb>& left_child = accel.nodes[left_child_idx];
        int32_t right_child_idx = node.right;
        const GpuBvhNode<BoundingVolumeType::kAabb>& right_child = accel.nodes[right_child_idx];

        float left_t_min, left_t_max, right_t_min, right_t_max;
        bool left_hit = IntersectRayAabb(ray, left_child.bounds, left_t_min, left_t_max);
        bool right_hit = IntersectRayAabb(ray, right_child.bounds, right_t_min, right_t_max);
        rt_stats.traversal_count += 2;

        if (left_t_min > t_hit || left_t_max <= kEpsilon) {
            left_hit = false;
        }
        if (right_t_min > t_hit || right_t_max <= kEpsilon) {
            right_hit = false;
        };

        if (left_hit && right_hit) {
            if (left_t_min < right_t_min) {
                stack[++stack_top] = right_child_idx;
                stack[++stack_top] = left_child_idx;
            } else {
                stack[++stack_top] = left_child_idx;
                stack[++stack_top] = right_child_idx;
            }
        } else if (left_hit) {
            stack[++stack_top] = left_child_idx;
        } else if (right_hit) {
            stack[++stack_top] = right_child_idx;
        }

        assert(stack_top < 64 && "Intersect(): stack overflow");
    }

    if (!hit) return false;

    ray_hit.t = t_hit;
    ray_hit.triangle_id = tri_hit;
    ray_hit.material_id = scene.tri_mat_id[tri_hit];
    ray_hit.pos = RayAt(ray, t_hit);
    ray_hit.b0 = 1.0f - b1_hit - b2_hit;
    ray_hit.b1 = b1_hit;
    ray_hit.b2 = b2_hit;
    ray_hit.epsilon = RayEpsilon(ray_hit.pos, ray_hit.t);
    ray_hit.geom_normal = scene.tri_geom_norm[tri_hit];
    ray_hit.normal = scene.tri_geom_norm[tri_hit];
    if (scene.tri_has_vn[tri_hit]) {
        ray_hit.normal = Normalize(scene.tri_v0_norm[tri_hit] * ray_hit.b0 + scene.tri_v1_norm[tri_hit] * ray_hit.b1 +
                                   scene.tri_v2_norm[tri_hit] * ray_hit.b2);
    }

    return true;
}

template <>
D bool BvhAcceleration<BoundingVolumeType::kAabb>::IntersectAny(RaytracingStats& rt_stats, const GpuSceneView& scene,
                                                                const GpuRay& ray, bool backface_culling) const {
    rt_stats.query_count++;
    int32_t stack[64];
    int32_t stack_top = 0;
    stack[stack_top] = *accel.root;

    while (stack_top != -1) {
        const GpuBvhNode<BoundingVolumeType::kAabb>& node = accel.nodes[stack[stack_top--]];

        // Leaf node
        if (node.is_leaf) {
            rt_stats.intersection_count++;
            int32_t tri_idx = accel.tri_idxs[node.left];
            float b1, b2;
            float t = IntersectRayTriangle(scene.tri_v0_pos[tri_idx], scene.tri_v1_pos[tri_idx],
                                           scene.tri_v2_pos[tri_idx], ray, b1, b2, backface_culling);
            if (t > kEpsilon && t < ray.t_max) {
                return true;  // Any hit found
            }
            continue;
        }

        // Internal node
        int32_t left_child_idx = node.left;
        const GpuBvhNode<BoundingVolumeType::kAabb>& left_child = accel.nodes[left_child_idx];
        int32_t right_child_idx = node.right;
        const GpuBvhNode<BoundingVolumeType::kAabb>& right_child = accel.nodes[right_child_idx];

        float t_min, t_max;
        if (IntersectRayAabb(ray, left_child.bounds, t_min, t_max) && t_max > kEpsilon) {
            rt_stats.traversal_count++;
            stack[++stack_top] = left_child_idx;
        }
        if (IntersectRayAabb(ray, right_child.bounds, t_min, t_max) && t_max > kEpsilon) {
            rt_stats.traversal_count++;
            stack[++stack_top] = right_child_idx;
        }
    }

    return false;
}

template <>
D bool BvhAcceleration<BoundingVolumeType::kSobb>::Intersect(RaytracingStats& rt_stats, const GpuSceneView& scene,
                                                             const GpuRay& ray, GpuRayHit& ray_hit,
                                                             bool backface_culling) const {
    rt_stats.query_count++;
    printf("BvhAcceleration::kSobb: Intersect() not implemented yet!\n");
    return false;
}

template <>
D bool BvhAcceleration<BoundingVolumeType::kSobb>::IntersectAny(RaytracingStats& rt_stats, const GpuSceneView& scene,
                                                                const GpuRay& ray, bool backface_culling) const {
    rt_stats.query_count++;
    printf("BvhAcceleration::kSobb: IntersectAny() not implemented yet!\n");
    return false;
}

}  // namespace diplodocus::cuda_kernels

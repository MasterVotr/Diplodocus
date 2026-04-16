#include "acceleration/sbvh.h"

#include <numeric>
#include <queue>
#include <span>

#include "config/acceleration_structure_config.h"
#include "scene/ray.h"
#include "scene/ray_hit.h"
#include "scene/triangle.h"
#include "util/logger.h"
#include "util/timer.h"
#include "util/util.h"

namespace diplodocus {

void SBvh::Build(const AccelerationStructureConfig& accel_config, Stats& stats, std::span<const Triangle> triangles) {
    triangles_ = triangles;
    Logger::debug("Building SAH BVH...");
    Timer build_t;

    // Clear nodes_ and tri_idxs
    size_t n = triangles_.size();
    nodes_.resize(2 * n - 1);
    triangle_indices_.resize(n);
    std::iota(triangle_indices_.begin(), triangle_indices_.end(), 0);
    Timer init_t;

    // Precalculate tbs and cbs
    std::vector<AABB> tbs;  // Triangle bboxes
    tbs.reserve(n);
    std::vector<Vec3> tcs;  // Triangle centroids
    tcs.reserve(n);
    std::for_each(triangles_.begin(), triangles_.end(), [&](const auto& t) {
        tbs.emplace_back(CalculateTriangleAabb(t));
        tcs.emplace_back(CalculateTriangleCentroid(t));
    });

    // Create root and launch recursive subdivide
    AABB vb;
    AABB cb;
    std::for_each(tbs.begin(), tbs.end(), [&](const auto& tb) { vb.Expand(tb); });
    std::for_each(tcs.begin(), tcs.end(), [&](const auto& tc) { cb.Expand(tc); });

    Logger::debug("Init time: {} ms", init_t.elapsed_ms());

    BvhNode& root = nodes_[0];
    root.t_begin = 0;
    root.aabb_min = vb.min;
    root.aabb_max = vb.max;
    root.t_count = n;
    next_node_idx_ = 1;

    Subdivide(accel_config, 0, 0, cb, tcs, tbs);
    stats.accel_stats.build_time = build_t.elapsed_ms();

    CalculateStats(stats);
}

bool SBvh::Intersect(Stats& stats, const Ray& ray, RayHit& ray_hit, bool backface_culling) const {
    stats.accel_stats.query_count++;

    float best_t = ray.t_max;
    bool found_hit = false;

    struct TraversalNode {
        size_t node_idx;
        float dist_to_aabb;
    };

    auto cmp = [](const TraversalNode& a, const TraversalNode& b) {
        return a.dist_to_aabb > b.dist_to_aabb;  // min-heap: smallest distance first
    };
    std::priority_queue<TraversalNode, std::vector<TraversalNode>, decltype(cmp)> pq(cmp);
    pq.emplace(0, 0.0f);

    while (!pq.empty()) {
        auto [node_idx, _] = pq.top();
        pq.pop();

        const BvhNode& node = nodes_[node_idx];
        stats.accel_stats.traversal_count += 1;

        // Get ray-AABB intersection
        float t_min, t_max;
        if (!IntersectRayAabb(ray, {node.aabb_min, node.aabb_max}, t_min, t_max) || t_min >= best_t) {
            continue;
        }

        if (node.is_leaf()) {
            // Intersect all triangles in leaf
            float t_hit = best_t;
            int tri_hit = -1;
            float b1_hit = 0, b2_hit = 0;

            for (size_t i = 0; i < node.t_count; i++) {
                int tri_idx = triangle_indices_[node.t_begin + i];
                float b1, b2;
                float t = triangles_[tri_idx].IntersectRay(ray, b1, b2, backface_culling);
                stats.accel_stats.intersection_count += 1;

                if (t > kEpsilon && t < t_hit) {
                    t_hit = t;
                    tri_hit = tri_idx;
                    b1_hit = b1;
                    b2_hit = b2;
                }
            }

            if (tri_hit != -1) {
                best_t = t_hit;
                found_hit = true;

                // Update ray_hit
                const Triangle& triangle = triangles_[tri_hit];
                ray_hit.t = t_hit;
                ray_hit.triangle_id = tri_hit;
                ray_hit.material_id = triangle.material_id;
                ray_hit.pos = ray.At(t_hit);
                ray_hit.b0 = 1.0f - b1_hit - b2_hit;
                ray_hit.b1 = b1_hit;
                ray_hit.b2 = b2_hit;
                ray_hit.epsilon = RayEpsilon(ray_hit.pos, ray_hit.t);
                ray_hit.geom_normal = triangle.geom_normal;
                ray_hit.normal = triangle.geom_normal;
                if (triangle.has_vertex_normals) {
                    ray_hit.normal = Normalize(triangle.v0.normal * ray_hit.b0 + triangle.v1.normal * ray_hit.b1 +
                                               triangle.v2.normal * ray_hit.b2);
                }
            }
        } else {
            // Internal node: traverse closer child first
            size_t left_child_idx = node.t_begin;
            size_t right_child_idx = left_child_idx + 1;
            const BvhNode& left_child = nodes_[left_child_idx];
            const BvhNode& right_child = nodes_[right_child_idx];

            // Get distances to both children
            float left_t_min, left_t_max, right_t_min, right_t_max;
            bool left_hit = IntersectRayAabb(ray, {left_child.aabb_min, left_child.aabb_max}, left_t_min, left_t_max);
            bool right_hit =
                IntersectRayAabb(ray, {right_child.aabb_min, right_child.aabb_max}, right_t_min, right_t_max);

            // Add to priority queue: closer child processed first
            if (left_hit && left_t_max > kEpsilon) {
                pq.emplace(left_child_idx, std::max(0.0f, left_t_min));
            }
            if (right_hit && right_t_max > kEpsilon) {
                pq.emplace(right_child_idx, std::max(0.0f, right_t_min));
            }
        }
    }

    return found_hit;
}

bool SBvh::IntersectAny(Stats& stats, const Ray& ray, bool backface_culling) const {
    stats.accel_stats.query_count++;

    std::stack<size_t> s;
    s.emplace(0);

    while (!s.empty()) {
        size_t node_idx = s.top();
        s.pop();

        const BvhNode& node = nodes_[node_idx];
        stats.accel_stats.traversal_count += 1;

        // Test AABB intersection
        float t_min, t_max;
        if (!IntersectRayAabb(ray, {node.aabb_min, node.aabb_max}, t_min, t_max)) {
            continue;
        }

        if (node.is_leaf()) {
            // Test triangles: return on first hit
            for (size_t i = 0; i < node.t_count; i++) {
                int tri_idx = triangle_indices_[node.t_begin + i];
                float b1, b2;
                float t = triangles_[tri_idx].IntersectRay(ray, b1, b2, backface_culling);
                stats.accel_stats.intersection_count += 1;

                if (t > kEpsilon && t < ray.t_max) {
                    return true;  // Any hit found
                }
            }
        } else {
            // Internal node: push both children
            size_t left_child_idx = node.t_begin;
            size_t right_child_idx = left_child_idx + 1;
            s.emplace(left_child_idx);
            s.emplace(right_child_idx);
        }
    }

    return false;
}

float SBvh::FindBestSplit(const AccelerationStructureConfig& accel_config, BvhNode& node, int& axis, float& split_pos,
                          const AABB& cb, AABB& TB_L, AABB& TB_R, const std::vector<Vec3>& tcs,
                          const std::vector<AABB>& tbs) {
    int bin_count = accel_config.bin_count;
    // Decide the longest cb axis
    axis = 0;
    if (cb.size.x >= cb.size.y && cb.size.x >= cb.size.z) {
        axis = 0;
    } else if (cb.size.y >= cb.size.z) {
        axis = 1;
    } else {
        axis = 2;
    }

    // Calculate bins
    Timer t_bins;
    std::vector<AABB> bbs(bin_count, AABB());
    std::vector<size_t> ns(bin_count, 0);

    float k_0 = cb.min[axis];
    float k_1 = bin_count * (1 - 1e-3) /
                cb.size[axis];  // Possible problem with bin_idx being K (bin_count) - epsilon was too big
    size_t t_end = node.t_begin + node.t_count;
    for (size_t t = node.t_begin; t < t_end; t++) {
        size_t t_idx = triangle_indices_[t];
        size_t bin_idx = std::min(bin_count - 1, static_cast<int>(k_1 * (tcs[t_idx][axis] - k_0)));

        bbs[bin_idx].Expand(tbs[t_idx]);
        ns[bin_idx]++;
    }

    // Prefix sum calculation left->right
    float best_split_cost = kInfinity;
    int split_count = bin_count - 1;
    std::vector<size_t> N_Ls(split_count), N_Rs(split_count);
    std::vector<AABB> TB_Ls(split_count), TB_Rs(split_count);
    std::vector<float> A_Ls(split_count), A_Rs(split_count);

    N_Ls[0] = ns[0];
    TB_Ls[0] = bbs[0];
    A_Ls[0] = TB_Ls[0].SurfaceArea();

    for (int i = 1; i < split_count; i++) {
        N_Ls[i] = N_Ls[i - 1] + ns[i];
        TB_Ls[i] = TB_Ls[i - 1];
        TB_Ls[i].Expand(bbs[i]);
        A_Ls[i] = TB_Ls[i].SurfaceArea();
    }

    // Prefix sum calculation right->left and best split
    N_Rs[split_count - 1] = ns[split_count];
    TB_Rs[split_count - 1] = bbs[split_count];
    A_Rs[split_count - 1] = TB_Rs[split_count - 1].SurfaceArea();

    float scale = cb.size[axis] / bin_count;  // size of one bin in axis
    for (int i = split_count - 2; i >= 0; i--) {
        N_Rs[i] = N_Rs[i + 1] + ns[i + 1];
        TB_Rs[i] = TB_Rs[i + 1];
        TB_Rs[i].Expand(bbs[i + 1]);
        A_Rs[i] = TB_Rs[i].SurfaceArea();

        // Best split
        float split_cost = N_Ls[i] * A_Ls[i] + N_Rs[i] * A_Rs[i];
        if (split_cost != 0 && split_cost < best_split_cost) {
            best_split_cost = split_cost;
            split_pos = cb.min[axis] + scale * (i + 1);

            TB_L = TB_Ls[i];
            TB_R = TB_Rs[i];
        }
    }

    return best_split_cost;
}

void SBvh::Subdivide(const AccelerationStructureConfig& accel_config, size_t node_idx, int depth, AABB cb,
                     const std::vector<Vec3>& tcs, const std::vector<AABB>& tbs) {
    BvhNode& node = nodes_[node_idx];

    // Check for BVH criteria
    bool cb_too_small = cb.size.x < kEpsilon && cb.size.y < kEpsilon && cb.size.z < kEpsilon;
    if (node.t_count < accel_config.max_triangles_per_BB || depth >= accel_config.max_depth || cb_too_small) {
        return;
    }

    // Find best split
    int axis;
    float split_pos;
    AABB TB_L, TB_R;
    FindBestSplit(accel_config, node, axis, split_pos, cb, TB_L, TB_R, tcs, tbs);

    // Triangle partitioning
    Timer t_partitioning;
    int i = node.t_begin;
    int j = node.t_begin + node.t_count - 1;
    while (i <= j) {
        if (tcs[triangle_indices_[i]][axis] < split_pos) {
            i++;
        } else {
            std::swap(triangle_indices_[i], triangle_indices_[j]);
            j--;
        }
    }

    // Recalculate N_L and N_R based on actual partitioning
    size_t N_L = i - node.t_begin;
    size_t N_R = node.t_count - N_L;

    // Abort split if one of the children is empty
    if (N_L == 0 || N_R == 0) {
        return;
    }

    // Create child nodes
    size_t left_child_idx = next_node_idx_++;
    nodes_[left_child_idx].t_begin = node.t_begin;
    nodes_[left_child_idx].t_count = N_L;
    nodes_[left_child_idx].aabb_min = TB_L.min;
    nodes_[left_child_idx].aabb_max = TB_L.max;

    size_t right_child_idx = next_node_idx_++;
    nodes_[right_child_idx].t_begin = node.t_begin + N_L;
    nodes_[right_child_idx].t_count = N_R;
    nodes_[right_child_idx].aabb_min = TB_R.min;
    nodes_[right_child_idx].aabb_max = TB_R.max;

    // Set current node to internal
    node.t_begin = left_child_idx;
    node.t_count = 0;

    // Calculate new centroid bboxes
    AABB CB_L;
    size_t t_start_L = nodes_[left_child_idx].t_begin;
    size_t t_end_L = nodes_[left_child_idx].t_begin + nodes_[left_child_idx].t_count;
    for (size_t i = t_start_L; i < t_end_L; i++) {
        CB_L.Expand(tcs[triangle_indices_[i]]);
    }

    AABB CB_R;
    size_t t_start_R = nodes_[right_child_idx].t_begin;
    size_t t_end_R = nodes_[right_child_idx].t_begin + nodes_[right_child_idx].t_count;
    for (size_t i = t_start_R; i < t_end_R; i++) {
        CB_R.Expand(tcs[triangle_indices_[i]]);
    }

    // Subdivide recursively
    Subdivide(accel_config, left_child_idx, depth + 1, CB_L, tcs, tbs);
    Subdivide(accel_config, right_child_idx, depth + 1, CB_R, tcs, tbs);
}

void SBvh::CalculateStats(Stats& stats) const {
    std::stack<std::pair<size_t, size_t>> s;
    s.emplace(0, 0);
    while (!s.empty()) {
        auto [node_idx, depth] = s.top();
        s.pop();

        const BvhNode& node = nodes_[node_idx];
        stats.accel_stats.node_count++;

        if (node.is_leaf()) {
            stats.accel_stats.leaf_node_count++;
            continue;
        }

        stats.accel_stats.inner_node_count++;

        size_t left_child_idx = node.t_begin;
        size_t right_child_idx = left_child_idx + 1;
        s.emplace(left_child_idx, depth + 1);
        s.emplace(right_child_idx, depth + 1);
    }

    stats.accel_stats.memory_consumption = nodes_.size() * sizeof(BvhNode);
}

}  // namespace diplodocus

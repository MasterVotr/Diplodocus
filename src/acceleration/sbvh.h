#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "acceleration/aabb.h"
#include "acceleration/acceleration_structure.h"
#include "config/acceleration_structure_config.h"
#include "scene/triangle.h"
#include "stats/stats.h"
#include "util/vec3.h"

namespace diplodocus {

class SBvh : public AccelerationStructure {
   public:
    void Build(const AccelerationStructureConfig& acceleration_structure_config, Stats& stats,
               std::span<const Triangle> triangles) override;
    bool Intersect(Stats& stats, const Ray& ray, RayHit& ray_hit, bool backface_culling) const override;
    bool IntersectAny(Stats& stats, const Ray& ray, bool backface_culling) const override;

   private:
    struct BvhNode {
        Vec3 aabb_min = {kInfinity, kInfinity, kInfinity};
        Vec3 aabb_max = {-kInfinity, -kInfinity, -kInfinity};
        int32_t t_begin = 0;  // or left child (right child is t_begin+1)
        int32_t t_count = 0;
        bool is_leaf() const { return t_count > 0; }
    };

    std::span<const Triangle> triangles_;
    std::vector<int32_t> triangle_indices_;
    std::vector<BvhNode> nodes_;
    size_t next_node_idx_;

    float FindBestSplit(const AccelerationStructureConfig& accel_config, BvhNode& node, int& axis, float& split_pos,
                        const AABB& cb, AABB& TB_L, AABB& TB_R, const std::vector<Vec3>& tcs,
                        const std::vector<AABB>& tbs);
    void Subdivide(const AccelerationStructureConfig& accel_config, size_t node_idx, int depth, AABB cb,
                   const std::vector<Vec3>& tcs, const std::vector<AABB>& tbs);
    void UpdateNodeBounds(size_t node_idx, const std::vector<Vec3>& tbs);

    void CalculateStats(Stats& stats) const;
};

}  // namespace diplodocus

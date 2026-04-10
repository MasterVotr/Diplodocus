#pragma once

#include <span>

#include "config/acceleration_structure_config.h"
#include "scene/ray_hit.h"
#include "scene/triangle.h"
#include "stats/stats.h"

namespace diplodocus {

class AccelerationStructure {
   public:
    virtual ~AccelerationStructure() = default;
    virtual void Build(const AccelerationStructureConfig& acceleration_structure_config, Stats& stats,
                       std::span<const Triangle> triangles) = 0;
    virtual bool Intersect(Stats& stats, const Ray& ray, RayHit& ray_hit, bool backface_culling) const = 0;
    virtual bool IntersectAny(Stats& stats, const Ray& ray, bool backface_culling) const = 0;
};

}  // namespace diplodocus

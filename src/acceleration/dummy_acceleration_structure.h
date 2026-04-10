#pragma once

#include <span>

#include "acceleration/acceleration_structure.h"
#include "scene/triangle.h"

namespace diplodocus {

class DummyAccelerationStucture : public AccelerationStructure {
   public:
    void Build(const AccelerationStructureConfig& acceleration_structure_config, Stats& stats,
               std::span<const Triangle> triangles) override;
    bool Intersect(Stats& stats, const Ray& ray, RayHit& ray_hit, bool backface_culling) const override;
    bool IntersectAny(Stats& stats, const Ray& ray, bool backface_culling) const override;

   private:
    std::span<const Triangle> triangles_;
};

}  // namespace diplodocus

#pragma once

#include "gpu/acceleration/gpu_acceleration_structure.h"
#include "gpu/config/gpu_acceleration_structure_config.h"
#include "gpu/scene/gpu_ray.h"
#include "gpu/scene/gpu_scene.h"

namespace diplodocus::cuda_kernels {

// Forward decalaration
struct GpuRayHit;

struct NoAcceleration {
    D bool Intersect(const GpuSceneView& scene, const GpuRay& ray, GpuRayHit& ray_hit, bool backface_culling) const;
    D bool IntersectAny(const GpuSceneView& scene, const GpuRay& ray, bool backface_culling) const;
};

template <BoundingVolumeType BV>
struct BvhAcceleration {
    GpuBvhView<BV> accel;

    D bool Intersect(const GpuSceneView& scene, const GpuRay& ray, GpuRayHit& ray_hit, bool backface_culling) const;
    D bool IntersectAny(const GpuSceneView& scene, const GpuRay& ray, bool backface_culling) const;
};

}  // namespace diplodocus::cuda_kernels

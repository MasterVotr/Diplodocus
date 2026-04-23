#pragma once

#include "gpu/acceleration/gpu_acceleration_structure.h"
#include "gpu/config/gpu_acceleration_structure_config.h"
#include "gpu/scene/gpu_scene.h"
#include "stats/construction_stats.h"

namespace diplodocus::cuda_kernels {

struct GpuBuildParams {
    GpuAccelerationStructureConfig accel_config;
    GpuSceneView scene;
    ConstructionStats& construction_stats;
};

template <BoundingVolumeType BV, MortonType M>
void LaunchBuildBvhKernelsImpl(const GpuBuildParams& params, GpuBvhView<BV> bvh);

template <BoundingVolumeType BV>
void LaunchBuildBvhKernels(const GpuBuildParams& params, GpuBvhView<BV> bvh) {
    switch (params.accel_config.morton_type) {
        case MortonType::kMorton32:
            LaunchBuildBvhKernelsImpl<BV, MortonType::kMorton32>(params, bvh);
            break;
        case MortonType::kEmc64Var1:
            LaunchBuildBvhKernelsImpl<BV, MortonType::kEmc64Var1>(params, bvh);
            break;
    }
}

}  // namespace diplodocus::cuda_kernels

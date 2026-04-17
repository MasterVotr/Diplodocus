#pragma once

#include "gpu/acceleration/gpu_acceleration_structure.h"
#include "gpu/config/gpu_acceleration_structure_config.h"
#include "gpu/scene/gpu_scene.h"

namespace diplodocus::cuda_kernels {

struct GpuBuildParams {
    GpuAccelerationStructureConfig accel_config;
    GpuSceneView scene;
};

template <BoundingVolumeType BV, MortonType M>
void LaunchBuildBvhKernelsImpl(const GpuBuildParams& params, GpuBvhView<BV> bvh);

template <BoundingVolumeType BV>
void LaunchBuildBvhKernels(const GpuBuildParams& params, GpuBvhView<BV> bvh) {
    switch (params.accel_config.morton_type) {
        case MortonType::kMorton30:
            LaunchBuildBvhKernelsImpl<BV, MortonType::kMorton30>(params, bvh);
            break;
        case MortonType::kEmc60:
            LaunchBuildBvhKernelsImpl<BV, MortonType::kEmc60>(params, bvh);
            break;
    }
}

}  // namespace diplodocus::cuda_kernels

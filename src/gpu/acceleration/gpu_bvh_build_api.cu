#include <cstdio>

#include "gpu/acceleration/gpu_bvh_build_api.h"
#include "gpu/config/gpu_acceleration_structure_config.h"

namespace diplodocus::cuda_kernels {

template <>
void LaunchBuildBvhKernelsImpl<BoundingVolumeType::kAabb, MortonType::kMorton30>(
    const GpuBuildParams& params, GpuBvhView<BoundingVolumeType::kAabb> bvh) {
    printf("Building PLOC\n");
    // TODO
    printf("Building not implemented yet!\n");
}

template <>
void LaunchBuildBvhKernelsImpl<BoundingVolumeType::kAabb, MortonType::kEmc60>(
    const GpuBuildParams& params, GpuBvhView<BoundingVolumeType::kAabb> bvh) {
    printf("Building PLOC + EMC\n");
    // TODO
    printf("Building not implemented yet!\n");
}

template <>
void LaunchBuildBvhKernelsImpl<BoundingVolumeType::kSobb, MortonType::kMorton30>(
    const GpuBuildParams& params, GpuBvhView<BoundingVolumeType::kSobb> bvh) {
    printf("Building PLOC + SOBB\n");
    // TODO
    printf("Building not implemented yet!\n");
}

template <>
void LaunchBuildBvhKernelsImpl<BoundingVolumeType::kSobb, MortonType::kEmc60>(
    const GpuBuildParams& params, GpuBvhView<BoundingVolumeType::kSobb> bvh) {
    printf("Building PLOC + EMC + SOBB\n");
    // TODO
    printf("Building not implemented yet!\n");
}

}  // namespace diplodocus::cuda_kernels

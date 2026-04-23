#pragma once

namespace diplodocus::cuda_kernels {

enum class MortonType { kMorton32, kEmc64Var1, kEmc64Var2 };

enum class BoundingVolumeType { kAabb, kSobb };

struct GpuAccelerationStructureConfig {
    MortonType morton_type = MortonType::kMorton32;
    BoundingVolumeType bounding_volmue_type = BoundingVolumeType::kAabb;
    int max_triangles_per_leaf = 4;
    int nn_search_radius = 10;
    int kdop_size = 32;
};

}  // namespace diplodocus::cuda_kernels

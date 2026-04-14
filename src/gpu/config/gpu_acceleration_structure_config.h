#pragma once

namespace diplodocus::cuda_kernels {

enum class MortonType { kMorton30, kEmc60 };

enum class BoundingVolumeType { kAabb, kSobb };

struct GpuAccelerationStructureConfig {
    MortonType morton_type = MortonType::kMorton30;
    BoundingVolumeType bounding_volmue_type = BoundingVolumeType::kAabb;
    int max_triangles_in_leaf = 4;
    int nn_search_radius = 10;
    int kdop_size = 32;
};

}  // namespace diplodocus::cuda_kernels

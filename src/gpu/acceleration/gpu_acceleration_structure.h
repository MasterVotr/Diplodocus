#pragma once

#include <vector_types.h>

#include <cstdint>
#include <vector>

#include "gpu/acceleration/gpu_aabb.h"
#include "gpu/acceleration/gpu_sobb.h"
#include "gpu/config/gpu_acceleration_structure_config.h"
#include "gpu/cuda_buffer.h"

namespace diplodocus::cuda_kernels {

template <BoundingVolumeType BV>
struct GpuBvhNode {};

template <>
struct GpuBvhNode<BoundingVolumeType::kAabb> {
    GpuAabb bounds;
    int32_t parent;
    int32_t left;   // t_begin/tri_idx if leaf
    int32_t right;  // t_count if leaf
    int32_t is_leaf;
};

template <>
struct GpuBvhNode<BoundingVolumeType::kSobb> {
    GpuSobb bounds;
    int32_t parent;
    int32_t left;   // t_begin/tri_idx if leaf
    int32_t right;  // t_count if leaf
    int32_t is_leaf;
};

template <BoundingVolumeType BV>
struct GpuBvhView {
    GpuBvhNode<BV>* nodes;
    int node_count{0};
    int32_t* tri_idxs;
    int tri_count{0};
    int32_t* root;
};

template <BoundingVolumeType BV>
class GpuBvh {
   public:
    GpuBvh(int node_count, int tri_count) {
        nodes.Allocate(node_count);
        tri_idxs.Allocate(tri_count);
        root.Allocate();
    }
    GpuBvh(const GpuBvh&) = delete;
    GpuBvh& operator=(const GpuBvh&) = delete;

    GpuBvhView<BV> GetView() {
        return GpuBvhView<BV>{
            nodes.Data(), static_cast<int>(nodes.Size()), tri_idxs.Data(), static_cast<int>(tri_idxs.Size()),
            root.Data(),
        };
    }
    void Download(std::vector<GpuBvhNode<BV>>& nodes_cpu_dst, std::vector<int32_t>& tri_idxs_cpu_dst,
                  int32_t& root_cpu_dst) const {
        nodes.Download(nodes_cpu_dst);
        tri_idxs.Download(tri_idxs_cpu_dst);
        root_cpu_dst = root.Download();
    }

    CudaBuffer<GpuBvhNode<BV>> nodes;
    CudaBuffer<int32_t> tri_idxs;
    CudaValue<int32_t> root;
};

static_assert(sizeof(GpuBvhNode<BoundingVolumeType::kAabb>) == 40, "GpuBvhNode<kAabb>: unexpected sizeof()");

}  // namespace diplodocus::cuda_kernels

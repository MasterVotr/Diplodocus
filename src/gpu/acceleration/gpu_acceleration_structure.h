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
    int32_t t_leaf;
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
        nodes_.Allocate(node_count);
        tri_idxs_.Allocate(tri_count);
        root_.Allocate();
    }
    GpuBvh(const GpuBvh&) = delete;
    GpuBvh& operator=(const GpuBvh&) = delete;

    GpuBvhView<BV> GetView() {
        return GpuBvhView<BV>{
            nodes_.Data(), static_cast<int>(nodes_.Size()), tri_idxs_.Data(), static_cast<int>(tri_idxs_.Size()),
            root_.Data(),
        };
    }
    void Download(std::vector<GpuBvhNode<BV>>& nodes_cpu_dst, std::vector<int32_t>& tri_idxs_cpu_dst) const {
        nodes_.Download(nodes_cpu_dst);
        tri_idxs_.Download(tri_idxs_cpu_dst);
    }

   private:
    CudaBuffer<GpuBvhNode<BV>> nodes_;
    CudaBuffer<int32_t> tri_idxs_;
    CudaValue<int32_t> root_;
};

static_assert(sizeof(GpuBvhNode<BoundingVolumeType::kAabb>) == 40, "GpuBvhNode<kAabb>: unexpected sizeof()");

}  // namespace diplodocus::cuda_kernels

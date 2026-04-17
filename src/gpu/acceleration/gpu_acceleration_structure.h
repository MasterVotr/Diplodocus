#pragma once

#include <vector_types.h>

#include <cstdint>
#include <variant>

#include "gpu/config/gpu_acceleration_structure_config.h"
#include "gpu/cuda_buffer.h"

namespace diplodocus::cuda_kernels {

struct GpuAabb {
    float3 bb_min;
    float3 bb_max;
};

struct GpuSobb {
    // TODO
};

template <BoundingVolumeType BV>
struct GpuBvhNode {};

template <>
struct GpuBvhNode<BoundingVolumeType::kAabb> {
    GpuAabb bounds;
    int32_t t_begin;
    int32_t t_count;
};

template <>
struct GpuBvhNode<BoundingVolumeType::kSobb> {
    GpuSobb bounds;
    int32_t t_begin;
    int32_t t_count;
};

template <BoundingVolumeType BV>
struct GpuBvhView {
    GpuBvhNode<BV>* nodes;
    int node_count{0};
    int32_t* tri_idxs;
    int tri_count{0};
};

template <BoundingVolumeType BV>
class GpuBvh {
   public:
    GpuBvh(int node_count, int tri_count) {
        nodes_.Allocate(node_count);
        tri_idxs_.Allocate(tri_count);
    }
    GpuBvh(const GpuBvh&) = delete;
    GpuBvh& operator=(const GpuBvh&) = delete;

    GpuBvhView<BV> GetView() {
        return GpuBvhView<BV>{
            nodes_.Data(),
            static_cast<int>(nodes_.Size()),
            tri_idxs_.Data(),
            static_cast<int>(tri_idxs_.Size()),
        };
    }

   private:
    CudaBuffer<GpuBvhNode<BV>> nodes_;
    CudaBuffer<int32_t> tri_idxs_;
};

}  // namespace diplodocus::cuda_kernels

#include <cuda_runtime_api.h>
#include <driver_types.h>
#include <thrust/device_ptr.h>
#include <thrust/gather.h>
#include <thrust/sequence.h>
#include <thrust/sort.h>
#include <vector_functions.h>

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cub/block/block_reduce.cuh>
#include <cub/device/device_reduce.cuh>
#include <cub/device/device_scan.cuh>
#include <thrust/detail/scan.inl>

#include "gpu/acceleration/gpu_aabb.h"
#include "gpu/acceleration/gpu_aabb_ops.h"
#include "gpu/acceleration/gpu_acceleration_structure.h"
#include "gpu/acceleration/gpu_bvh_build_api.h"
#include "gpu/acceleration/gpu_morton.h"
#include "gpu/config/gpu_acceleration_structure_config.h"
#include "gpu/cuda_buffer.h"
#include "gpu/cuda_math.h"
#include "gpu/cuda_utils.h"
#include "gpu/scene/gpu_scene.h"
#include "util/timer.h"

namespace diplodocus::cuda_kernels {

constexpr int kBvhBuildThreadsPerBlock = 256;

__global__ void CalculateAabbsKernel(GpuSceneView scene, int tri_count, GpuAabb* aabbs) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= tri_count) return;

    aabbs[idx].min = Fmin(scene.tri_v0_pos[idx], Fmin(scene.tri_v1_pos[idx], scene.tri_v2_pos[idx]));
    aabbs[idx].max = Fmax(scene.tri_v0_pos[idx], Fmax(scene.tri_v1_pos[idx], scene.tri_v2_pos[idx]));
}

__global__ void CalculateMortonsKernel(int tri_count, GpuAabb* aabbs, GpuAabb* scene_aabb, uint32_t* mcodes) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= tri_count) return;

    float3 centroid = (aabbs[idx].min + aabbs[idx].max) * 0.5f;
    float3 scene_size = scene_aabb->max - scene_aabb->min;
    float3 normalized_centroid;
    normalized_centroid.x = scene_size.x > kEpsilon ? (centroid.x - scene_aabb->min.x) / scene_size.x : 0.5f;
    normalized_centroid.y = scene_size.y > kEpsilon ? (centroid.y - scene_aabb->min.y) / scene_size.y : 0.5f;
    normalized_centroid.z = scene_size.z > kEpsilon ? (centroid.z - scene_aabb->min.z) / scene_size.z : 0.5f;
    mcodes[idx] = Morton30(normalized_centroid);
}

__global__ void InitLeavesKernel(int32_t tri_count, GpuBvhNode<BoundingVolumeType::kAabb>* nodes, GpuAabb* aabbs,
                                 int32_t* node_idx) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= tri_count) return;

    nodes[idx].bounds = aabbs[idx];
    nodes[idx].left = idx;
    nodes[idx].right = -1;
    nodes[idx].parent = -1;
    nodes[idx].is_leaf = 1;

    node_idx[idx] = idx;
}

__global__ void FindNearestNeighborKernel(int32_t n, GpuAabb* aabbs, int32_t* nns, int radius) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n) return;

    // TODO load radius (neighborhood) into shared memory
    float best_cost = kInfinity;
    int best_neighbor = -1;
    int begin = Fmax(idx - radius, 0);
    int end = Fmin(idx + radius, n - 1);
    for (int j = begin; j <= end; j++) {
        float cost = CalculateAabbSurfaceArea(MergeAabb(aabbs[idx], aabbs[j]));
        if (idx != j && best_cost > cost) {
            best_cost = cost;
            best_neighbor = j;
        }
    }
    nns[idx] = best_neighbor;
}

__global__ void MatchAndClassifyKernel(int32_t n, int32_t* nns, int32_t* valid_flags, int32_t* leader_flags) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n) return;

    int j = nns[idx];

    // Cluster does not have a nearest neighbor - should not happen
    assert(j != -1 && "MatchAndClassifyKernel: Cluster does not have a nearest neighbor");

    int32_t leader = nns[j] == idx && idx < j;
    int32_t single = nns[j] != idx;
    valid_flags[idx] = leader || single;
    leader_flags[idx] = leader;
}

__global__ void MergeAndCompactKernel(int32_t n, GpuAabb* aabbs, int32_t* node_idxs, int32_t* nns,
                                      int32_t* valid_offsets, int32_t* valid_flags, int32_t* leader_offsets,
                                      int32_t* leader_flags, GpuBvhNode<BoundingVolumeType::kAabb>* nodes,
                                      int32_t base_node_offset, GpuAabb* aabbs_next, int32_t* node_idxs_next) {
    int c_i = blockIdx.x * blockDim.x + threadIdx.x;
    if (c_i >= n) return;

    // Merge follower - discard
    if (!valid_flags[c_i]) return;

    int c_j = nns[c_i];

    // Calculate the offset for new cluster inserting
    int cluster_offset_next = valid_offsets[c_i];

    // Merge leader - create new node
    if (leader_flags[c_i]) {
        int new_node = base_node_offset + leader_offsets[c_i];
        int left = node_idxs[c_i];
        int right = node_idxs[c_j];

        GpuAabb merged_aabb = MergeAabb(aabbs[c_i], aabbs[c_j]);

        // Create new node
        nodes[new_node].bounds = merged_aabb;
        nodes[new_node].left = left;
        nodes[new_node].right = right;
        nodes[new_node].is_leaf = 0;
        nodes[new_node].parent = -1;

        nodes[left].parent = new_node;
        nodes[right].parent = new_node;

        aabbs_next[cluster_offset_next] = merged_aabb;
        node_idxs_next[cluster_offset_next] = new_node;
    }
    // Single -> carry over
    else {
        aabbs_next[cluster_offset_next] = aabbs[c_i];
        node_idxs_next[cluster_offset_next] = node_idxs[c_i];
    }
}

inline void UpdateCompactionCount(const CudaBuffer<int32_t>& offsets, const CudaBuffer<int32_t>& flags, int32_t& count,
                                  int32_t n) {
    int32_t offset_last = 0;
    int32_t flag_last = 0;

    CUDA_CHECK(cudaMemcpy(&offset_last, offsets.Data() + (n - 1), sizeof(int32_t), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(&flag_last, flags.Data() + (n - 1), sizeof(int32_t), cudaMemcpyDeviceToHost));

    count = offset_last + flag_last;
}

template <>
void LaunchBuildBvhKernelsImpl<BoundingVolumeType::kAabb, MortonType::kMorton30>(
    const GpuBuildParams& params, GpuBvhView<BoundingVolumeType::kAabb> bvh) {
    if (bvh.tri_count == 0) {
        printf("BvhBuild: No triangles to build.\n");
        return;
    }

    int block_count = (bvh.tri_count + kBvhBuildThreadsPerBlock - 1) / kBvhBuildThreadsPerBlock;
    Timer init_t;
    Timer timer;

    // Initialize triangle indexes
    thrust::device_ptr<int32_t> thrustp_tri_idxs = thrust::device_pointer_cast(bvh.tri_idxs);
    thrust::sequence(thrustp_tri_idxs, thrustp_tri_idxs + bvh.tri_count);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
    params.construction_stats.kernel_time += timer.elapsed_ns();

    // Triangle bboxes
    CudaBuffer<GpuAabb> aabbs;  // Cin (bounds)
    aabbs.Allocate(bvh.tri_count);
    timer.reset();
    CalculateAabbsKernel<<<block_count, kBvhBuildThreadsPerBlock>>>(params.scene, bvh.tri_count, aabbs.Data());
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
    params.construction_stats.kernel_time += timer.elapsed_ns();

    // Scene bbox using cub::reduce
    CudaValue<GpuAabb> scene_aabb;
    void* d_tmp_storage = nullptr;
    size_t d_tmp_storage_bytes = 0;
    GpuAabb empty_aabb{Splat(kInfinity), Splat(-kInfinity)};
    timer.reset();
    cub::DeviceReduce::Reduce(d_tmp_storage, d_tmp_storage_bytes, aabbs.Data(), scene_aabb.Data(), bvh.tri_count,
                              ExpandAabb(), empty_aabb);
    CUDA_CHECK(cudaMalloc(&d_tmp_storage, d_tmp_storage_bytes));
    cub::DeviceReduce::Reduce(d_tmp_storage, d_tmp_storage_bytes, aabbs.Data(), scene_aabb.Data(), bvh.tri_count,
                              ExpandAabb(), empty_aabb);
    CUDA_CHECK(cudaFree(d_tmp_storage));
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
    params.construction_stats.kernel_time += timer.elapsed_ns();

    // Morton codes
    CudaBuffer<uint32_t> mcodes;
    mcodes.Allocate(bvh.tri_count);
    timer.reset();
    CalculateMortonsKernel<<<block_count, kBvhBuildThreadsPerBlock>>>(bvh.tri_count, aabbs.Data(), scene_aabb.Data(),
                                                                      mcodes.Data());
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
    params.construction_stats.kernel_time += timer.elapsed_ns();

    // Sort Morton codes
    thrust::device_ptr<uint32_t> d_mcodes = thrust::device_pointer_cast(mcodes.Data());
    timer.reset();
    thrust::stable_sort_by_key(d_mcodes, d_mcodes + bvh.tri_count, thrustp_tri_idxs);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
    params.construction_stats.kernel_time += timer.elapsed_ns();

    // Main while loop
    CudaBuffer<int32_t> node_idxs;       // Cin (indices)
    CudaBuffer<GpuAabb> aabbs_next;      // Cout (bounds)
    CudaBuffer<int32_t> node_idxs_next;  // Cout (indices)
    CudaBuffer<int32_t> nns;             // N
    CudaBuffer<int32_t> valid_offsets;   // P
    CudaBuffer<int32_t> valid_flags;     // 0 for merge-follower, 1 for merge-leader and single
    CudaBuffer<int32_t> leader_offsets;  // P (merged nodes)
    CudaBuffer<int32_t> leader_flags;    // 1 for merge-leader, 0 for others
    node_idxs.Allocate(bvh.tri_count);
    aabbs_next.Allocate(bvh.tri_count);
    node_idxs_next.Allocate(bvh.tri_count);
    nns.Allocate(bvh.tri_count);
    valid_offsets.Allocate(bvh.tri_count);
    valid_flags.Allocate(bvh.tri_count);
    leader_offsets.Allocate(bvh.tri_count);
    leader_flags.Allocate(bvh.tri_count);
    int32_t valid_cnt{0};
    int32_t leader_cnt{0};

    // Rearange aabbs by tri_idxs (morton codes)
    thrust::device_ptr<GpuAabb> thrustp_aabbs(aabbs.Data());
    thrust::device_ptr<GpuAabb> thrustp_aabbs_next(aabbs_next.Data());
    thrust::gather(thrustp_tri_idxs, thrustp_tri_idxs + bvh.tri_count, thrustp_aabbs, thrustp_aabbs_next);
    cuda::std::swap(aabbs, aabbs_next);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    // Setup leaves on bvh
    timer.reset();
    InitLeavesKernel<<<block_count, kBvhBuildThreadsPerBlock>>>(bvh.tri_count, bvh.nodes, aabbs.Data(),
                                                                node_idxs.Data());
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
    params.construction_stats.kernel_time += timer.elapsed_ns();

    params.construction_stats.init_time = init_t.elapsed_ns();

    int radius = params.accel_config.nn_search_radius;
    int current_n = bvh.tri_count;
    int base_node_offset = current_n;
    while (current_n > 1) {
        // printf("BvhBuild::Ploc: while loop round start: current_n = %d\n", current_n);
        block_count = (current_n + kBvhBuildThreadsPerBlock - 1) / kBvhBuildThreadsPerBlock;

        // Nearest neighbor
        timer.reset();
        FindNearestNeighborKernel<<<block_count, kBvhBuildThreadsPerBlock>>>(current_n, aabbs.Data(), nns.Data(),
                                                                             radius);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        auto elapsed_ns = timer.elapsed_ns();
        params.construction_stats.nn_search_time += elapsed_ns;
        params.construction_stats.kernel_time += elapsed_ns;

        // Merge
        timer.reset();
        MatchAndClassifyKernel<<<block_count, kBvhBuildThreadsPerBlock>>>(current_n, nns.Data(), valid_flags.Data(),
                                                                          leader_flags.Data());
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        elapsed_ns = timer.elapsed_ns();
        params.construction_stats.match_and_classify_time += elapsed_ns;
        params.construction_stats.kernel_time += elapsed_ns;

        // Compaction
        timer.reset();
        thrust::exclusive_scan(thrust::device, valid_flags.Data(), valid_flags.Data() + current_n,
                               valid_offsets.Data());
        thrust::exclusive_scan(thrust::device, leader_flags.Data(), leader_flags.Data() + current_n,
                               leader_offsets.Data());
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        elapsed_ns = timer.elapsed_ns();
        params.construction_stats.prefix_scan_time += elapsed_ns;
        params.construction_stats.kernel_time += elapsed_ns;

        timer.reset();
        UpdateCompactionCount(valid_offsets, valid_flags, valid_cnt, current_n);
        UpdateCompactionCount(leader_offsets, leader_flags, leader_cnt, current_n);
        params.construction_stats.memcopy_time += timer.elapsed_ns();
        assert(leader_cnt > 0 && "BvhBuild::Ploc: At least one merge has to happen each round");

        timer.reset();
        MergeAndCompactKernel<<<block_count, kBvhBuildThreadsPerBlock>>>(
            current_n, aabbs.Data(), node_idxs.Data(), nns.Data(), valid_offsets.Data(), valid_flags.Data(),
            leader_offsets.Data(), leader_flags.Data(), bvh.nodes, base_node_offset, aabbs_next.Data(),
            node_idxs_next.Data());
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        elapsed_ns = timer.elapsed_ns();
        params.construction_stats.merge_and_compact_time += elapsed_ns;
        params.construction_stats.kernel_time += elapsed_ns;

        base_node_offset += leader_cnt;

        cuda::std::swap(aabbs, aabbs_next);
        cuda::std::swap(node_idxs, node_idxs_next);

        current_n = valid_cnt;
        // printf("BvhBuild::Ploc: while loop round results: valid_cnt = %d, merge_cnt = %d\n", valid_cnt, leader_cnt);
    }

    timer.reset();
    cudaMemcpy(bvh.root, node_idxs.Data() + 0, sizeof(*bvh.root), cudaMemcpyDeviceToDevice);
    params.construction_stats.merge_and_compact_time += timer.elapsed_ns();

    // TODO: Collapse leaves
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

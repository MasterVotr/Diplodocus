#include <cuda_runtime_api.h>
#include <device_atomic_functions.h>
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
#include "gpu/acceleration/gpu_sobb.h"
#include "gpu/acceleration/gpu_sobb_ops.h"
#include "gpu/config/gpu_acceleration_structure_config.h"
#include "gpu/cuda_buffer.h"
#include "gpu/cuda_math.h"
#include "gpu/cuda_utils.h"
#include "gpu/scene/gpu_scene.h"
#include "util/timer.h"

namespace diplodocus::cuda_kernels {

namespace {

constexpr int kBvhBuildThreadsPerBlock = 256;

inline void UpdateCompactionCount(const CudaBuffer<int32_t>& offsets, const CudaBuffer<int32_t>& flags, int32_t& count,
                                  int32_t n) {
    int32_t offset_last = 0;
    int32_t flag_last = 0;

    CUDA_CHECK(cudaMemcpy(&offset_last, offsets.Data() + (n - 1), sizeof(int32_t), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(&flag_last, flags.Data() + (n - 1), sizeof(int32_t), cudaMemcpyDeviceToHost));

    count = offset_last + flag_last;
}

G void CalculateAabbsKernel(GpuSceneView scene, int tri_count, GpuAabb* aabbs) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= tri_count) return;

    aabbs[idx].min = Fmin(scene.tri_v0_pos[idx], Fmin(scene.tri_v1_pos[idx], scene.tri_v2_pos[idx]));
    aabbs[idx].max = Fmax(scene.tri_v0_pos[idx], Fmax(scene.tri_v1_pos[idx], scene.tri_v2_pos[idx]));
}

template <MortonType M>
G void CalculateMortonsKernel(int tri_count, typename MortonTrait<M>::Setup setup, GpuAabb* aabbs,
                              typename MortonTrait<M>::CodeT* mcodes) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= tri_count) return;

    mcodes[idx] = MortonTrait<M>::Encode(aabbs[idx], setup);
}

G void InitLeavesKernel(int32_t tri_count, GpuBvhNode<BoundingVolumeType::kAabb>* nodes, GpuAabb* aabbs,
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

G void FindNearestNeighborKernel(int32_t n, GpuAabb* aabbs, int32_t* nns, int radius) {
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

G void MatchAndClassifyKernel(int32_t n, int32_t* nns, int32_t* valid_flags, int32_t* leader_flags) {
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

G void MergeAndCompactKernel(int32_t n, GpuAabb* aabbs, int32_t* node_idxs, int32_t* nns, int32_t* valid_offsets,
                             int32_t* valid_flags, int32_t* leader_offsets, int32_t* leader_flags,
                             GpuBvhNode<BoundingVolumeType::kAabb>* nodes, int32_t base_node_offset,
                             GpuAabb* aabbs_next, int32_t* node_idxs_next) {
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

G void SobbRefitKernel(GpuSceneView scene, int node_cnt, GpuBvhNode<BoundingVolumeType::kAabb>* old_nodes,
                       GpuBvhNode<BoundingVolumeType::kSobb>* new_nodes, GpuDop* dops, int32_t* tri_idxs,
                       int32_t* ready_children) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= node_cnt) return;
    if (!old_nodes[idx].is_leaf) return;

    GpuBvhNode<BoundingVolumeType::kAabb> node = old_nodes[idx];
    int tri_idx = tri_idxs[node.left];

    // Accumulation k-DOP for the whole bottom up traversal
    GpuDop dop = CreateDopFromTriangle(scene.tri_v0_pos[tri_idx], scene.tri_v1_pos[tri_idx], scene.tri_v2_pos[tri_idx]);

    // Leaf k-DOP from primitive/s
    dops[idx] = dop;
    new_nodes[idx].bounds = CreateSobbFromDopK2(dops[idx]);

    // Copy information from old aabb node into new sobb node
    new_nodes[idx].is_leaf = 1;
    new_nodes[idx].left = node.left;
    new_nodes[idx].right = -1;
    new_nodes[idx].parent = node.parent;

    // Iteration variables for bottom-up traversal
    int c0_idx = idx;
    int node_idx = node.parent;

    // Break looop when at root or when first child to traverse to parent node
    while (42) {
        // First child increments and get back a 0, second child increments and gets back a 1
        int second_child = atomicAdd(&ready_children[node_idx], 1);
        if (!second_child) return;

        node = old_nodes[node_idx];

        // Copy information from old aabb node into new sobb node
        new_nodes[node_idx].is_leaf = 0;
        new_nodes[node_idx].left = node.left;
        new_nodes[node_idx].right = node.right;
        new_nodes[node_idx].parent = node.parent;

        // Get the child that is not yet acculumated in dop
        int c1_idx = (c0_idx == node.left) ? node.right : node.left;

        // Combine children k-DOPs into new sobb
        FitDop2Dop(dop, dops[c1_idx]);
        dops[node_idx] = dop;
        new_nodes[node_idx].bounds = CreateSobbFromDopK2(dop);
        // new_nodes[node_idx].bounds = CreateSobbFromAabb(node.bounds);

        // Sync before signaling parent node that current is ready
        __threadfence();

        // Next iteration
        c0_idx = node_idx;
        node_idx = node.parent;

        if (node_idx == -1) return;
    }
}

}  // namespace

template <BoundingVolumeType BV, MortonType M>
void LaunchBuildBvhKernelsImpl(const GpuBuildParams& params, GpuBvh<BV>& bvh) {
    const int tri_count = bvh.tri_idxs.Size();
    const int node_count = bvh.nodes.Size();

    if (tri_count == 0) {
        printf("BvhBuild: No triangles to build bvh from.\n");
        return;
    }

    int block_count = (tri_count + kBvhBuildThreadsPerBlock - 1) / kBvhBuildThreadsPerBlock;

    Timer init_t;
    Timer timer;

    // Initialize triangle indexes
    timer.reset();
    thrust::device_ptr<int32_t> thrustp_tri_idxs = thrust::device_pointer_cast(bvh.tri_idxs.Data());
    thrust::sequence(thrustp_tri_idxs, thrustp_tri_idxs + tri_count);
    params.construction_stats.kernel_time += timer.elapsed_ns();

    // Triangle bboxes
    CudaBuffer<GpuAabb> aabbs;  // Cin (bounds)
    aabbs.Allocate(tri_count);
    timer.reset();
    CalculateAabbsKernel<<<block_count, kBvhBuildThreadsPerBlock>>>(params.scene, tri_count, aabbs.Data());
    params.construction_stats.kernel_time += timer.elapsed_ns();

    // Scene bbox using cub::reduce
    CudaValue<GpuAabb> scene_aabb;
    scene_aabb.Allocate();
    void* d_tmp_storage = nullptr;
    size_t d_tmp_storage_bytes = 0;
    GpuAabb empty_aabb{Splat(kInfinity), Splat(-kInfinity)};
    timer.reset();
    cub::DeviceReduce::Reduce(d_tmp_storage, d_tmp_storage_bytes, aabbs.Data(), scene_aabb.Data(), tri_count,
                              MergeAabbFunctor(), empty_aabb);
    CUDA_CHECK(cudaMalloc(&d_tmp_storage, d_tmp_storage_bytes));
    cub::DeviceReduce::Reduce(d_tmp_storage, d_tmp_storage_bytes, aabbs.Data(), scene_aabb.Data(), tri_count,
                              MergeAabbFunctor(), empty_aabb);
    CUDA_CHECK(cudaFree(d_tmp_storage));
    params.construction_stats.kernel_time += timer.elapsed_ns();

    // Morton codes
    CudaBuffer<typename MortonTrait<M>::CodeT> mcodes;
    CUDA_CHECK(cudaDeviceSynchronize());
    typename MortonTrait<M>::Setup msetup = MortonTrait<M>::Init(scene_aabb.Download());
    mcodes.Allocate(tri_count);
    timer.reset();
    CalculateMortonsKernel<M>
        <<<block_count, kBvhBuildThreadsPerBlock>>>(tri_count, msetup, aabbs.Data(), mcodes.Data());
    auto elapsed_ns = timer.elapsed_ns();
    params.construction_stats.kernel_time += elapsed_ns;
    params.construction_stats.morton_construction_time = elapsed_ns;
    // std::vector<typename MortonTrait<M>::CodeT> h_mcodes;

    // Sort Morton codes
    thrust::device_ptr<typename MortonTrait<M>::CodeT> d_mcodes = thrust::device_pointer_cast(mcodes.Data());
    timer.reset();
    thrust::stable_sort_by_key(d_mcodes, d_mcodes + tri_count, thrustp_tri_idxs);
    elapsed_ns = timer.elapsed_ns();
    params.construction_stats.kernel_time += elapsed_ns;
    params.construction_stats.morton_sort_time = elapsed_ns;

    // Main while loop
    CudaBuffer<GpuBvhNode<BoundingVolumeType::kAabb>> nodes;  // Working nodes
    CudaBuffer<int32_t> node_idxs;                            // Cin (indices)
    CudaBuffer<GpuAabb> aabbs_next;                           // Cout (bounds)
    CudaBuffer<int32_t> node_idxs_next;                       // Cout (indices)
    CudaBuffer<int32_t> nns;                                  // N
    CudaBuffer<int32_t> valid_offsets;                        // P
    CudaBuffer<int32_t> valid_flags;                          // 0 for merge-follower, 1 for merge-leader and single
    CudaBuffer<int32_t> leader_offsets;                       // P (merged nodes)
    CudaBuffer<int32_t> leader_flags;                         // 1 for merge-leader, 0 for others
    nodes.Allocate(node_count);
    node_idxs.Allocate(tri_count);
    aabbs_next.Allocate(tri_count);
    node_idxs_next.Allocate(tri_count);
    nns.Allocate(tri_count);
    valid_offsets.Allocate(tri_count);
    valid_flags.Allocate(tri_count);
    leader_offsets.Allocate(tri_count);
    leader_flags.Allocate(tri_count);
    int32_t valid_cnt{0};
    int32_t leader_cnt{0};

    // Rearange aabbs by tri_idxs (morton codes)
    thrust::device_ptr<GpuAabb> thrustp_aabbs(aabbs.Data());
    thrust::device_ptr<GpuAabb> thrustp_aabbs_next(aabbs_next.Data());
    thrust::gather(thrustp_tri_idxs, thrustp_tri_idxs + tri_count, thrustp_aabbs, thrustp_aabbs_next);
    cuda::std::swap(aabbs, aabbs_next);

    // Setup leaves on bvh
    timer.reset();
    InitLeavesKernel<<<block_count, kBvhBuildThreadsPerBlock>>>(tri_count, nodes.Data(), aabbs.Data(),
                                                                node_idxs.Data());
    params.construction_stats.kernel_time += timer.elapsed_ns();

    params.construction_stats.init_time = init_t.elapsed_ns();

    const int radius = params.accel_config.nn_search_radius;
    int current_n = tri_count;
    int base_node_offset = current_n;
    while (current_n > 1) {
        block_count = (current_n + kBvhBuildThreadsPerBlock - 1) / kBvhBuildThreadsPerBlock;

        // Nearest neighbor
        timer.reset();
        FindNearestNeighborKernel<<<block_count, kBvhBuildThreadsPerBlock>>>(current_n, aabbs.Data(), nns.Data(),
                                                                             radius);
        elapsed_ns = timer.elapsed_ns();
        params.construction_stats.nn_search_time += elapsed_ns;
        params.construction_stats.kernel_time += elapsed_ns;

        // Merge
        timer.reset();
        MatchAndClassifyKernel<<<block_count, kBvhBuildThreadsPerBlock>>>(current_n, nns.Data(), valid_flags.Data(),
                                                                          leader_flags.Data());
        elapsed_ns = timer.elapsed_ns();
        params.construction_stats.match_and_classify_time += elapsed_ns;
        params.construction_stats.kernel_time += elapsed_ns;

        // Compaction
        timer.reset();
        thrust::exclusive_scan(thrust::device, valid_flags.Data(), valid_flags.Data() + current_n,
                               valid_offsets.Data());
        thrust::exclusive_scan(thrust::device, leader_flags.Data(), leader_flags.Data() + current_n,
                               leader_offsets.Data());
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
            leader_offsets.Data(), leader_flags.Data(), nodes.Data(), base_node_offset, aabbs_next.Data(),
            node_idxs_next.Data());
        elapsed_ns = timer.elapsed_ns();
        params.construction_stats.merge_and_compact_time += elapsed_ns;
        params.construction_stats.kernel_time += elapsed_ns;

        base_node_offset += leader_cnt;

        cuda::std::swap(aabbs, aabbs_next);
        cuda::std::swap(node_idxs, node_idxs_next);

        current_n = valid_cnt;
    }

    timer.reset();
    CUDA_CHECK(
        cudaMemcpy(bvh.root.Data(), node_idxs.Data() + 0, sizeof(*bvh.GetView().root), cudaMemcpyDeviceToDevice));
    params.construction_stats.memcopy_time += timer.elapsed_ns();

    // Free up memory
    timer.reset();
    node_idxs.Free();
    node_idxs_next.Free();
    nns.Free();
    valid_offsets.Free();
    valid_flags.Free();
    leader_offsets.Free();
    leader_flags.Free();
    aabbs_next.Free();
    aabbs.Free();
    params.construction_stats.memcopy_time += timer.elapsed_ns();

    // SOBB refit
    if constexpr (BV == BoundingVolumeType::kAabb) {
        cuda::std::swap(bvh.nodes, nodes);
    } else if constexpr (BV == BoundingVolumeType::kSobb) {
        Timer sobb_refit_t;

        // Preapare ready_childern flag and k-DOPs
        timer.reset();
        CudaBuffer<int32_t> ready_children;
        ready_children.Allocate(node_count);
        CUDA_CHECK(cudaMemset(ready_children.Data(), 0, ready_children.Size() * sizeof(int32_t)));
        CudaBuffer<GpuDop> dops;
        dops.Allocate(node_count);
        params.construction_stats.memcopy_time += timer.elapsed_ns();

        // Launch refitting kernel
        timer.reset();
        block_count = (node_count + kBvhBuildThreadsPerBlock - 1) / kBvhBuildThreadsPerBlock;
        SobbRefitKernel<<<block_count, kBvhBuildThreadsPerBlock>>>(params.scene, tri_count, nodes.Data(),
                                                                   bvh.nodes.Data(), dops.Data(), bvh.tri_idxs.Data(),
                                                                   ready_children.Data());
        params.construction_stats.kernel_time += timer.elapsed_ns();
        params.construction_stats.sobb_refit_time = sobb_refit_t.elapsed_ns();
    }
    CUDA_CHECK(cudaDeviceSynchronize());

    return;
}

// Explicit symbol emission
template void LaunchBuildBvhKernelsImpl<BoundingVolumeType::kAabb, MortonType::kMorton64>(
    const GpuBuildParams&, GpuBvh<BoundingVolumeType::kAabb>&);
template void LaunchBuildBvhKernelsImpl<BoundingVolumeType::kAabb, MortonType::kEmc64Var1>(
    const GpuBuildParams&, GpuBvh<BoundingVolumeType::kAabb>&);
template void LaunchBuildBvhKernelsImpl<BoundingVolumeType::kAabb, MortonType::kEmc64Var2>(
    const GpuBuildParams&, GpuBvh<BoundingVolumeType::kAabb>&);
template void LaunchBuildBvhKernelsImpl<BoundingVolumeType::kSobb, MortonType::kMorton64>(
    const GpuBuildParams&, GpuBvh<BoundingVolumeType::kSobb>&);
template void LaunchBuildBvhKernelsImpl<BoundingVolumeType::kSobb, MortonType::kEmc64Var1>(
    const GpuBuildParams&, GpuBvh<BoundingVolumeType::kSobb>&);
template void LaunchBuildBvhKernelsImpl<BoundingVolumeType::kSobb, MortonType::kEmc64Var2>(
    const GpuBuildParams&, GpuBvh<BoundingVolumeType::kSobb>&);

}  // namespace diplodocus::cuda_kernels

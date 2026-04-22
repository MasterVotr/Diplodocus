#include <cuda_runtime.h>

#include <cstdint>
#include <cub/device/device_reduce.cuh>

#include "gpu/acceleration/gpu_intersection.h"
#include "gpu/cuda_buffer.h"
#include "gpu/cuda_math.h"
#include "gpu/cuda_utils.h"
#include "gpu/framebuffer/gpu_framebuffer.h"
#include "gpu/renderer/gpu_pathtracer.h"
#include "gpu/renderer/gpu_raytracer.h"
#include "gpu/renderer/gpu_renderer_api.h"
#include "gpu/scene/gpu_ray.h"
#include "stats/raytracing_stats.h"

namespace diplodocus::cuda_kernels {

__global__ void HelloCundaKernel() { printf("Hello from Cunda!\n"); }

__global__ void ClearFramebufferKernel(GpuFramebufferView framebuffer, float3 color) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int n = framebuffer.width * framebuffer.height;
    if (idx >= n) return;

    framebuffer.data[idx] = color;
}

// __global__ void RaytracingStackKernel(GpuTraceContext trace_ctx) {
//     int idx = blockIdx.x * blockDim.x + threadIdx.x;
//     int n = trace_ctx.framebuffer.width * trace_ctx.framebuffer.height;
//     if (idx >= n) return;
//     int pixel_x = idx % trace_ctx.framebuffer.width;
//     int pixel_y = idx / trace_ctx.framebuffer.width;
//
//     float3 ray_dir = Normalize(trace_ctx.p00 + (trace_ctx.qw * pixel_x) + (trace_ctx.qh * pixel_y));
//     GpuRay ray{trace_ctx.cam_pos, ray_dir, trace_ctx.cam_far};
//
//     GpuRayContext ray_ctx;
//     ray_ctx.ray = ray;
//     ray_ctx.pixel_x = pixel_x;
//     ray_ctx.pixel_y = pixel_y;
//     ray_ctx.depth = 0;
//     float3 pixel_color = TraceRay(trace_ctx, ray_ctx);
//
//     trace_ctx.framebuffer.data[idx] = pixel_color;
// }

template <typename Acceleration>
__global__ void PathtracingKernel(GpuTraceContext<Acceleration> trace_ctx, RaytracingStats* rt_stats) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int n = trace_ctx.framebuffer.width * trace_ctx.framebuffer.height;
    if (idx >= n) return;
    int pixel_x = idx % trace_ctx.framebuffer.width;
    int pixel_y = idx / trace_ctx.framebuffer.width;

    float3 ray_dir = Normalize(trace_ctx.p00 + (trace_ctx.qw * pixel_x) + (trace_ctx.qh * pixel_y));
    GpuRay ray{trace_ctx.cam_pos, ray_dir, trace_ctx.cam_far};

    GpuRayContext ray_ctx{ray, pixel_x, pixel_y, 0, rt_stats[idx]};
    float3 pixel_color = TracePath(trace_ctx, ray_ctx);

    trace_ctx.framebuffer.data[idx] = pixel_color;
}

void HelloCunda() {
    PrintCudaDiagnostics();
    CheckGpuMemory();
    HelloCundaKernel<<<1, 2>>>();
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
}

void LaunchClearFramebufferKernel(const GpuFramebufferView& framebuffer, float3 color) {
    int n = framebuffer.width * framebuffer.height;
    int threads = 256;
    int blocks = (n + threads - 1) / threads;
    ClearFramebufferKernel<<<blocks, threads>>>(framebuffer, color);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
}

// void LaunchRaytracingKernel(const GpuTraceContextNoBvh& trace_ctx) {
//     // TraceRayStack uses recursion, so increase per-thread stack size to avoid device stack overflow.
//     CUDA_CHECK(cudaDeviceSetLimit(cudaLimitStackSize, trace_ctx.render_config.max_depth * 2 * 1024));
//
//     int n = trace_ctx.framebuffer.width * trace_ctx.framebuffer.height;
//     int threads = 256;
//     int blocks = (n + threads - 1) / threads;
//     RaytracingStackKernel<<<blocks, threads>>>(trace_ctx);
//     CUDA_CHECK(cudaGetLastError());
//     CUDA_CHECK(cudaDeviceSynchronize());
// }

struct MergeRaytracingStats {
    HD RaytracingStats operator()(const RaytracingStats& a, const RaytracingStats& b) const {
        return {
            a.frame_time + b.frame_time,
            a.raytracing_time + b.raytracing_time,
            a.primary_ray_count + b.primary_ray_count,
            a.secondary_ray_count + b.secondary_ray_count,
            a.shadow_ray_count + b.shadow_ray_count,
            a.query_count + b.query_count,
            a.intersection_count + b.intersection_count,
            a.traversal_count + b.traversal_count,
        };
    }
};

RaytracingStats ReduceStats(const CudaBuffer<RaytracingStats>& d_rt_stats_all, int n) {
    CudaValue<RaytracingStats> d_rt_stats;
    d_rt_stats.Allocate();

    void* d_tmp_storage = nullptr;
    size_t d_tmp_storage_bytes = 0;
    RaytracingStats init;
    cub::DeviceReduce::Reduce(d_tmp_storage, d_tmp_storage_bytes, d_rt_stats_all.Data(), d_rt_stats.Data(), n,
                              MergeRaytracingStats(), init);
    CUDA_CHECK(cudaMalloc(&d_tmp_storage, d_tmp_storage_bytes));
    cub::DeviceReduce::Reduce(d_tmp_storage, d_tmp_storage_bytes, d_rt_stats_all.Data(), d_rt_stats.Data(), n,
                              MergeRaytracingStats(), init);
    CUDA_CHECK(cudaFree(d_tmp_storage));
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    return d_rt_stats.Download();
}

template <typename Acceleration>
void LaunchPathtracingKernelImpl(const GpuTraceContext<Acceleration>& trace_ctx, RaytracingStats& rt_stats) {
    int n = trace_ctx.framebuffer.width * trace_ctx.framebuffer.height;

    // Crete rt_stats for each thread
    CudaBuffer<RaytracingStats> d_rt_stats_all;
    d_rt_stats_all.Allocate(n);
    CUDA_CHECK(cudaMemset(d_rt_stats_all.Data(), 0, n * sizeof(RaytracingStats)));

    int threads = 256;
    int blocks = (n + threads - 1) / threads;
    PathtracingKernel<Acceleration><<<blocks, threads>>>(trace_ctx, d_rt_stats_all.Data());
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    // Reduce rt_stats
    rt_stats = ReduceStats(d_rt_stats_all, n);
}

template <>
void LaunchPathtracingKernel(const GpuTraceContext<NoAcceleration>& trace_ctx, RaytracingStats& rt_stats) {
    LaunchPathtracingKernelImpl<NoAcceleration>(trace_ctx, rt_stats);
}

template <>
void LaunchPathtracingKernel(const GpuTraceContext<BvhAcceleration<BoundingVolumeType::kAabb>>& trace_ctx,
                             RaytracingStats& rt_stats) {
    LaunchPathtracingKernelImpl<BvhAcceleration<BoundingVolumeType::kAabb>>(trace_ctx, rt_stats);
}

template <>
void LaunchPathtracingKernel(const GpuTraceContext<BvhAcceleration<BoundingVolumeType::kSobb>>& trace_ctx,
                             RaytracingStats& rt_stats) {
    LaunchPathtracingKernelImpl<BvhAcceleration<BoundingVolumeType::kSobb>>(trace_ctx, rt_stats);
}

}  // namespace diplodocus::cuda_kernels

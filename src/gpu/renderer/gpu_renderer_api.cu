#include <cuda_runtime.h>

#include <cub/device/device_reduce.cuh>

#include "gpu/acceleration/gpu_intersection.h"
#include "gpu/cuda_buffer.h"
#include "gpu/cuda_math.h"
#include "gpu/cuda_utils.h"
#include "gpu/framebuffer/gpu_framebuffer.h"
#include "gpu/renderer/gpu_pathtracer.h"
#include "gpu/renderer/gpu_renderer_api.h"
#include "gpu/scene/gpu_ray.h"
#include "stats/raytracing_stats.h"

namespace diplodocus::cuda_kernels {

namespace {

HDI RaytracingStats MergeRaytracingStats(const RaytracingStats& a, const RaytracingStats& b) {
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

struct MergeRaytracingStatsFunctor {
    HD RaytracingStats operator()(const RaytracingStats& a, const RaytracingStats& b) const {
        return MergeRaytracingStats(a, b);
    }
};

__global__ void ClearFramebufferKernel(GpuFramebufferView framebuffer, float3 color) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int n = framebuffer.width * framebuffer.height;
    if (idx >= n) return;

    framebuffer.data[idx] = color;
}

template <typename Acceleration>
__global__ void PathtracingKernel(GpuTraceContext<Acceleration> trace_ctx, RaytracingStats* rt_stats) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int n = trace_ctx.framebuffer.width * trace_ctx.framebuffer.height;
    if (idx >= n) return;

    float3 pixel_color = Splat(0.0f);
    int pixel_x = idx % trace_ctx.framebuffer.width;
    int pixel_y = idx / trace_ctx.framebuffer.width;
    float3 ray_dir = Normalize(trace_ctx.p00 + (trace_ctx.qw * pixel_x) + (trace_ctx.qh * pixel_y));
    GpuRay ray{trace_ctx.cam_pos, ray_dir, trace_ctx.cam_far};

    // Sample given pixel
    int pixel_sample_cnt = trace_ctx.render_config.pixel_sample_cnt;
    for (int pixel_s = 0; pixel_s < pixel_sample_cnt; pixel_s++) {
        RaytracingStats pixel_stats;
        GpuRayContext ray_ctx{ray, pixel_x, pixel_y, pixel_s, 0, pixel_stats};

        pixel_color = pixel_color + TracePath(trace_ctx, ray_ctx);
        rt_stats[idx] = MergeRaytracingStats(rt_stats[idx], pixel_stats);
    }
    pixel_color = pixel_color / pixel_sample_cnt;

    // Gamma correction
    float gamma = trace_ctx.render_config.gamma_correction;
    pixel_color = Fmax(pixel_color, Splat(0.0f));
    pixel_color = Pow(pixel_color, 1.0f / gamma);

    trace_ctx.framebuffer.data[idx] = pixel_color;
}

}  // namespace

void LaunchClearFramebufferKernel(const GpuFramebufferView& framebuffer, float3 color) {
    int n = framebuffer.width * framebuffer.height;
    int threads = 256;
    int blocks = (n + threads - 1) / threads;
    ClearFramebufferKernel<<<blocks, threads>>>(framebuffer, color);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
}

RaytracingStats ReduceStats(const CudaBuffer<RaytracingStats>& d_rt_stats_all, int n) {
    CudaValue<RaytracingStats> d_rt_stats;
    d_rt_stats.Allocate();

    void* d_tmp_storage = nullptr;
    size_t d_tmp_storage_bytes = 0;
    RaytracingStats init;
    cub::DeviceReduce::Reduce(d_tmp_storage, d_tmp_storage_bytes, d_rt_stats_all.Data(), d_rt_stats.Data(), n,
                              MergeRaytracingStatsFunctor(), init);
    CUDA_CHECK(cudaMalloc(&d_tmp_storage, d_tmp_storage_bytes));
    cub::DeviceReduce::Reduce(d_tmp_storage, d_tmp_storage_bytes, d_rt_stats_all.Data(), d_rt_stats.Data(), n,
                              MergeRaytracingStatsFunctor(), init);
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

#include <cuda_runtime.h>

#include "gpu/acceleration/gpu_intersection.h"
#include "gpu/cuda_math.h"
#include "gpu/cuda_utils.h"
#include "gpu/framebuffer/gpu_framebuffer.h"
#include "gpu/renderer/gpu_pathtracer.h"
#include "gpu/renderer/gpu_raytracer.h"
#include "gpu/renderer/gpu_renderer_api.h"
#include "gpu/scene/gpu_ray.h"

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
__global__ void PathtracingKernel(GpuTraceContext<Acceleration> trace_ctx) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int n = trace_ctx.framebuffer.width * trace_ctx.framebuffer.height;
    if (idx >= n) return;
    int pixel_x = idx % trace_ctx.framebuffer.width;
    int pixel_y = idx / trace_ctx.framebuffer.width;

    float3 ray_dir = Normalize(trace_ctx.p00 + (trace_ctx.qw * pixel_x) + (trace_ctx.qh * pixel_y));
    GpuRay ray{trace_ctx.cam_pos, ray_dir, trace_ctx.cam_far};

    GpuRayContext ray_ctx;
    ray_ctx.ray = ray;
    ray_ctx.pixel_x = pixel_x;
    ray_ctx.pixel_y = pixel_y;
    ray_ctx.depth = 0;
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

template <typename Acceleration>
void LaunchPathtracingKernelImpl(const GpuTraceContext<Acceleration>& trace_ctx) {
    int n = trace_ctx.framebuffer.width * trace_ctx.framebuffer.height;
    int threads = 256;
    int blocks = (n + threads - 1) / threads;
    PathtracingKernel<Acceleration><<<blocks, threads>>>(trace_ctx);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
}

template <>
void LaunchPathtracingKernel(const GpuTraceContext<NoAcceleration>& trace_ctx) {
    LaunchPathtracingKernelImpl<NoAcceleration>(trace_ctx);
}

template <>
void LaunchPathtracingKernel(const GpuTraceContext<BvhAcceleration<BoundingVolumeType::kAabb>>& trace_ctx) {
    LaunchPathtracingKernelImpl<BvhAcceleration<BoundingVolumeType::kAabb>>(trace_ctx);
}

template <>
void LaunchPathtracingKernel(const GpuTraceContext<BvhAcceleration<BoundingVolumeType::kSobb>>& trace_ctx) {
    LaunchPathtracingKernelImpl<BvhAcceleration<BoundingVolumeType::kSobb>>(trace_ctx);
}

}  // namespace diplodocus::cuda_kernels

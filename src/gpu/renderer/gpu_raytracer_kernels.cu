#include <cuda_runtime.h>

#include "gpu/cuda_math.h"
#include "gpu/cuda_utils.h"
#include "gpu/framebuffer/gpu_framebuffer.h"
#include "gpu/renderer/gpu_raytracer_impl.h"
#include "gpu/renderer/gpu_raytracer_kernels.h"
#include "gpu/scene/gpu_ray.h"

namespace diplodocus::cuda_kernels {

__global__ void HelloCundaKernel() { printf("Hello from Cunda!\n"); }

__global__ void ClearFramebufferKernel(GpuFramebufferView framebuffer, float3 color) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int n = framebuffer.width * framebuffer.height;
    if (idx >= n) return;

    framebuffer.data[idx] = color;
}

__global__ void RaytracingKernel(GpuTraceContext trace_ctx) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int n = trace_ctx.framebuffer.width * trace_ctx.framebuffer.height;
    if (idx >= n) return;
    int pixel_x = idx % trace_ctx.framebuffer.width;
    int pixel_y = idx / trace_ctx.framebuffer.width;

    float3 ray_dir = Normalize(trace_ctx.p00 + (trace_ctx.qw * pixel_x) + (trace_ctx.qh * pixel_y));
    GpuRay ray{trace_ctx.cam_pos, ray_dir, trace_ctx.cam_far};

    GpuRayContext ray_ctx{ray, pixel_x, pixel_y, 0};
    float3 pixel_color = TraceRay(trace_ctx, ray_ctx);

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

void LaunchRaytracingKernel(const GpuTraceContext& trace_ctx) {
    int n = trace_ctx.framebuffer.width * trace_ctx.framebuffer.height;
    int threads = 256;
    int blocks = (n + threads - 1) / threads;
    RaytracingKernel<<<blocks, threads>>>(trace_ctx);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
}

}  // namespace diplodocus::cuda_kernels

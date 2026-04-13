#pragma once

#include <cuda_runtime.h>

#include <cstdint>
#include <cstdio>

#include "gpu/cuda_compat.h"

namespace diplodocus::cuda_kernels {

inline void CheckCudaImpl(cudaError_t err, const char* expr, const char* file, int line) {
    if (err != cudaSuccess) {
        fprintf(stderr, "CUDA error: %s at %s:%d (%s)\n", cudaGetErrorString(err), file, line, expr);
        std::abort();
    }
}
#define CUDA_CHECK(expr) CheckCudaImpl((expr), #expr, __FILE__, __LINE__)

inline void CheckGpuMemory() {
    size_t freeB = 0, totalB = 0;
    CUDA_CHECK(cudaMemGetInfo(&freeB, &totalB));
    printf("CUDA mem: free %.2f MiB / total %.2f MiB\n", freeB / 1024.0 / 1024.0, totalB / 1024.0 / 1024.0);
}

inline void PrintCudaDiagnostics() {
    int deviceCount = 0;
    cudaError_t e = cudaGetDeviceCount(&deviceCount);
    if (e != cudaSuccess) {
        fprintf(stderr, "cudaGetDeviceCount failed: %s\n", cudaGetErrorString(e));
        return;
    }
    printf("CUDA devices: %d\n", deviceCount);
    for (int d = 0; d < deviceCount; ++d) {
        cudaDeviceProp prop{};
        CUDA_CHECK(cudaGetDeviceProperties(&prop, d));
        printf("Device %d: %s, totalGlobalMem = %.2f MiB\n", d, prop.name, prop.totalGlobalMem / 1024.0 / 1024.0);
    }

    // Force context creation on device 0
    CUDA_CHECK(cudaSetDevice(0));
    CUDA_CHECK(cudaFree(0));  // common trick to initialize context
}

// Hashing
DI uint32_t HashU32(uint32_t x) {
    // Avalanche hash
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

DI float U01FromU32(uint32_t x) {
    // Use the top 24 bits -> float mantisa
    return (x >> 8) * (1.0f / 16777216.0f);  // 2^24
}

}  // namespace diplodocus::cuda_kernels

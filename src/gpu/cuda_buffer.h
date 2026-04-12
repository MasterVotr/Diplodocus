#pragma once

#include <cuda_runtime.h>

#include <cstddef>
#include <span>
#include <vector>

#include "gpu/cuda_utils.h"

namespace diplodocus::cuda_kernels {

template <typename T>
class CudaBuffer {
   public:
    CudaBuffer() = default;
    ~CudaBuffer() { Free(); }
    CudaBuffer(const CudaBuffer<T>&) = delete;
    CudaBuffer& operator=(const CudaBuffer<T>&) = delete;

    void Allocate(size_t n) {
        Free();
        size = n;
        CUDA_CHECK(cudaMalloc(&data, size * sizeof(T)));
    }
    void Free() {
        if (data) cudaFree(data);
        size = 0;
        data = nullptr;
    }
    void Upload(std::span<const T> cpu_data) {
        if (cpu_data.size() != size) Allocate(cpu_data.size());
        CUDA_CHECK(cudaMemcpy(data, cpu_data.data(), size * sizeof(T), cudaMemcpyHostToDevice));
    }
    void Download(std::vector<T>& cpu_data) {
        cpu_data.resize(size);
        CUDA_CHECK(cudaMemcpy(cpu_data.data(), data, size * sizeof(T), cudaMemcpyDeviceToHost));
    }

    T* Data() { return data; }
    const T* Data() const { return data; }
    size_t Size() const { return size; }

   private:
    T* data{nullptr};
    size_t size{0};
};

}  // namespace diplodocus::cuda_kernels

#pragma once

#include <cuda_runtime.h>

#include <cstddef>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

#include "gpu/cuda_utils.h"

namespace diplodocus::cuda_kernels {

template <typename T>
class CudaValue {
   public:
    CudaValue() { Allocate(); }
    explicit CudaValue(const T& value) { Upload(value); }
    ~CudaValue() { Free(); }
    CudaValue(const CudaValue<T>&) = delete;
    CudaValue& operator=(const CudaValue<T>&) = delete;
    CudaValue(CudaValue<T>&& other) noexcept : data_(std::exchange(other.data_, nullptr)) {}
    CudaValue& operator=(CudaValue<T>&& other) noexcept {
        if (this != &other) {
            Free();
            data_ = std::exchange(other.data_, nullptr);
        }
        return *this;
    }

    void Allocate() {
        if (!data_) {
            CUDA_CHECK(cudaMalloc(&data_, sizeof(T)));
        }
    }
    void Free() {
        if (data_) cudaFree(data_);
        data_ = nullptr;
    }
    void Upload(const T& value) {
        Allocate();
        CUDA_CHECK(cudaMemcpy(data_, &value, sizeof(T), cudaMemcpyHostToDevice));
    }
    T Download() const {
        T value{};
        CUDA_CHECK(cudaMemcpy(&value, data_, sizeof(T), cudaMemcpyDeviceToHost));
        return value;
    }

    T* Data() { return data_; }
    const T* Data() const { return data_; }

    friend void swap(CudaValue& a, CudaValue& b) noexcept { std::swap(a.data_, b.data_); }

   private:
    T* data_{nullptr};
};

template <typename T>
class CudaBuffer {
   public:
    CudaBuffer() = default;
    ~CudaBuffer() { Free(); }
    CudaBuffer(const CudaBuffer<T>&) = delete;
    CudaBuffer& operator=(const CudaBuffer<T>&) = delete;
    CudaBuffer(CudaBuffer<T>&& other) noexcept
        : data_(std::exchange(other.data_, nullptr)), size_(std::exchange(other.size_, 0)) {}
    CudaBuffer& operator=(CudaBuffer<T>&& other) noexcept {
        if (this != &other) {
            Free();
            data_ = std::exchange(other.data_, nullptr);
            size_ = std::exchange(other.size_, 0);
        }
        return *this;
    }

    void Allocate(size_t n) {
        Free();
        size_ = n;
        CUDA_CHECK(cudaMalloc(&data_, size_ * sizeof(T)));
    }
    void Free() {
        if (data_) cudaFree(data_);
        size_ = 0;
        data_ = nullptr;
    }
    void Upload(std::span<const T> cpu_src) {
        if (cpu_src.size() != size_) Allocate(cpu_src.size());
        CUDA_CHECK(cudaMemcpy(data_, cpu_src.data(), size_ * sizeof(T), cudaMemcpyHostToDevice));
    }
    void Download(std::vector<T>& cpu_dst) const {
        if (cpu_dst.size() != size_) cpu_dst.resize(size_);
        CUDA_CHECK(cudaMemcpy(cpu_dst.data(), data_, size_ * sizeof(T), cudaMemcpyDeviceToHost));
    }
    void Download(T* cpu_dst, size_t n) const {
        if (n != size_) throw std::out_of_range("CudaBuffer::Download(): incorrect size of provided cpu_dst");
        CUDA_CHECK(cudaMemcpy(cpu_dst, data_, size_ * sizeof(T), cudaMemcpyDeviceToHost));
    }

    T* Data() { return data_; }
    const T* Data() const { return data_; }
    size_t Size() const { return size_; }

    friend void swap(CudaBuffer& a, CudaBuffer& b) noexcept {
        std::swap(a.data_, b.data_);
        std::swap(a.size_, b.size_);
    }

   private:
    T* data_{nullptr};
    size_t size_{0};
};

}  // namespace diplodocus::cuda_kernels

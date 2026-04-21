#pragma once

#include <vector_types.h>

#include "gpu/cuda_buffer.h"

namespace diplodocus {

// Forward declaration to avoid header polution
class Framebuffer;

namespace cuda_kernels {

struct GpuFramebufferView {
    float3* data;
    int width;
    int height;
};

class GpuFramebuffer {
   public:
    GpuFramebuffer() = default;
    GpuFramebuffer(const GpuFramebuffer&) = delete;
    GpuFramebuffer& operator=(const GpuFramebuffer&) = delete;

    void Resize(int width, int height);
    void Download(Framebuffer& framebuffer);

    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    GpuFramebufferView GetView() { return {data_.Data(), width_, height_}; }

   private:
    CudaBuffer<float3> data_;
    int width_{0};
    int height_{0};
};

static_assert(sizeof(float3) == 12);

}  // namespace cuda_kernels

}  // namespace diplodocus

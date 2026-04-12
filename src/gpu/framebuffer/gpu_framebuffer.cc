#include "gpu/framebuffer/gpu_framebuffer.h"

#include <cuda_runtime_api.h>
#include <driver_types.h>

#include "framebuffer/framebuffer.h"
#include "gpu/cuda_utils.h"

namespace diplodocus::cuda_kernels {

void GpuFramebuffer::Resize(int width, int height) {
    width_ = width;
    height_ = height;
    data_.Allocate(width_ * height_);
}

void GpuFramebuffer::Download(Framebuffer& framebuffer) {
    framebuffer.Resize(width_, height_);
    CUDA_CHECK(
        cudaMemcpy(framebuffer.GetDataPtr(), data_.Data(), width_ * height_ * sizeof(float3), cudaMemcpyDeviceToHost));
}

}  // namespace diplodocus::cuda_kernels

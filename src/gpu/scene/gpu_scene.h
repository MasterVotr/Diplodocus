#pragma once

#include <vector_types.h>

#include "gpu/cuda_buffer.h"

namespace diplodocus {

// Forward decalaration
class Scene;

namespace cuda_kernels {

struct GpuSceneView {
    // Triangles
    const float3* triangle_v0_pos;
    const float3* triangle_v1_pos;
    const float3* triangle_v2_pos;
    int triangle_count;
};

class GpuScene {
   public:
    GpuScene(const Scene& scene);
    GpuScene(const GpuScene&) = delete;
    GpuScene& operator=(const GpuScene&) = delete;

    GpuSceneView GetSceneView() const;

   private:
    // Triangles
    CudaBuffer<float3> triangle_v0_pos;
    CudaBuffer<float3> triangle_v1_pos;
    CudaBuffer<float3> triangle_v2_pos;

    // CudaBuffer<float3> triangle_v0_norm;
    // CudaBuffer<float3> triangle_v1_norm;
    // CudaBuffer<float3> triangle_v2_norm;

    // CudaBuffer<int> triangle_mat_id;

    // TODO: Materials

    // TODO: AreaLights

    // TODO: PointLights
};

}  // namespace cuda_kernels

}  // namespace diplodocus

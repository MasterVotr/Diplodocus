#pragma once

#include <vector_types.h>

#include <cstdint>

#include "gpu/cuda_buffer.h"

namespace diplodocus {

// Forward decalaration
class Scene;

namespace cuda_kernels {

struct GpuSceneView {
    // Triangles
    const float3* tri_v0_pos;
    const float3* tri_v1_pos;
    const float3* tri_v2_pos;
    const float3* tri_v0_norm;
    const float3* tri_v1_norm;
    const float3* tri_v2_norm;
    const float3* tri_geom_norm;
    const uint8_t* tri_has_vn;
    const int* tri_mat_id;
    const int tri_cnt;

    // Materials
    const float3* mat_ambient;
    const float3* mat_diffuse;
    const float3* mat_specular;
    const float3* mat_transmittance;
    const float3* mat_emission;
    const float* mat_shininess;
    const float* mat_ior;
    const float* mat_r_ior;
    const float* mat_dissolve;
    const int mat_cnt;

    // PointLights
    const float3* pl_pos;
    const float3* pl_color;
    const int pl_cnt;

    // AreaLights
    const float3* al_color;
    const int* al_tri_id;
    const float* al_surface_area;
    const int al_cnt;
};

class GpuScene {
   public:
    GpuScene(const Scene& scene);
    GpuScene(const GpuScene&) = delete;
    GpuScene& operator=(const GpuScene&) = delete;

    GpuSceneView GetView() const;

   private:
    // Triangles
    CudaBuffer<float3> tri_v0_pos_;
    CudaBuffer<float3> tri_v1_pos_;
    CudaBuffer<float3> tri_v2_pos_;
    CudaBuffer<float3> tri_v0_norm_;
    CudaBuffer<float3> tri_v1_norm_;
    CudaBuffer<float3> tri_v2_norm_;
    CudaBuffer<float3> tri_geom_norm_;
    CudaBuffer<uint8_t> tri_has_vn_;
    CudaBuffer<int> tri_mat_id_;

    // Materials
    CudaBuffer<float3> mat_ambient_;
    CudaBuffer<float3> mat_diffuse_;
    CudaBuffer<float3> mat_specular_;
    CudaBuffer<float3> mat_transmittance_;
    CudaBuffer<float3> mat_emission_;
    CudaBuffer<float> mat_shininess_;
    CudaBuffer<float> mat_ior_;
    CudaBuffer<float> mat_r_ior_;
    CudaBuffer<float> mat_dissolve_;

    // PointLights
    CudaBuffer<float3> pl_pos_;
    CudaBuffer<float3> pl_color_;

    // AreaLights
    CudaBuffer<float3> al_color_;
    CudaBuffer<int> al_tri_id_;
    CudaBuffer<float> al_surface_area_;
};

}  // namespace cuda_kernels

}  // namespace diplodocus

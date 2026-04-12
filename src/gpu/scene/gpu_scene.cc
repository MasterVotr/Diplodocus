#include "gpu/scene/gpu_scene.h"

#include <vector>

#include "scene/scene.h"

namespace diplodocus::cuda_kernels {

GpuScene::GpuScene(const Scene& scene) {
    // Trangsfer triangles
    const auto& tris = scene.Triangles();
    size_t t_cnt = scene.Triangles().size();

    std::vector<float3> t_v0(t_cnt), t_v1(t_cnt), t_v2(t_cnt);
    for (size_t i{0}; i < t_cnt; i++) {
        const auto& t = tris[i];
        t_v0[i] = make_float3(t.v0.pos.x, t.v0.pos.y, t.v0.pos.z);
        t_v1[i] = make_float3(t.v1.pos.x, t.v1.pos.y, t.v1.pos.z);
        t_v2[i] = make_float3(t.v2.pos.x, t.v2.pos.y, t.v2.pos.z);
    }
    triangle_v0_pos.Upload(t_v0);
    triangle_v1_pos.Upload(t_v1);
    triangle_v2_pos.Upload(t_v2);

    // Transfer materials

    // Transfer area lights

    // Transfer point lights
}

GpuSceneView GpuScene::GetSceneView() const {
    return GpuSceneView{
        triangle_v0_pos.Data(),
        triangle_v1_pos.Data(),
        triangle_v2_pos.Data(),
        static_cast<int>(triangle_v0_pos.Size()),
    };
}

}  // namespace diplodocus::cuda_kernels

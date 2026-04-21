#include "gpu/scene/gpu_scene.h"

#include <vector>

#include "scene/scene.h"

namespace diplodocus::cuda_kernels {

GpuScene::GpuScene(const Scene& scene) {
    // Trangsfer triangles
    const auto& tris = scene.Triangles();
    size_t t_cnt = tris.size();

    std::vector<float3> t_v0_pos(t_cnt), t_v1_pos(t_cnt), t_v2_pos(t_cnt);
    std::vector<float3> t_v0_norm(t_cnt), t_v1_norm(t_cnt), t_v2_norm(t_cnt);
    std::vector<float3> t_geom_norm(t_cnt);
    std::vector<uint8_t> t_has_vn(t_cnt);
    std::vector<int> t_mat_id(t_cnt);
    for (size_t i{0}; i < t_cnt; i++) {
        const auto& t = tris[i];
        t_v0_pos[i] = make_float3(t.v0.pos.x, t.v0.pos.y, t.v0.pos.z);
        t_v1_pos[i] = make_float3(t.v1.pos.x, t.v1.pos.y, t.v1.pos.z);
        t_v2_pos[i] = make_float3(t.v2.pos.x, t.v2.pos.y, t.v2.pos.z);
        t_v0_norm[i] = make_float3(t.v0.normal.x, t.v0.normal.y, t.v0.normal.z);
        t_v1_norm[i] = make_float3(t.v1.normal.x, t.v1.normal.y, t.v1.normal.z);
        t_v2_norm[i] = make_float3(t.v2.normal.x, t.v2.normal.y, t.v2.normal.z);
        t_geom_norm[i] = make_float3(t.geom_normal.x, t.geom_normal.y, t.geom_normal.z);
        t_has_vn[i] = t.has_vertex_normals;
        t_mat_id[i] = t.material_id;
    }
    tri_v0_pos_.Upload(t_v0_pos);
    tri_v1_pos_.Upload(t_v1_pos);
    tri_v2_pos_.Upload(t_v2_pos);
    tri_v0_norm_.Upload(t_v0_norm);
    tri_v1_norm_.Upload(t_v1_norm);
    tri_v2_norm_.Upload(t_v2_norm);
    tri_geom_norm_.Upload(t_geom_norm);
    tri_has_vn_.Upload(t_has_vn);
    tri_mat_id_.Upload(t_mat_id);

    // Transfer materials
    const auto& mats = scene.Materials();
    size_t m_cnt = mats.size();

    std::vector<float3> m_ambient(m_cnt), m_diffuse(m_cnt), m_specular(m_cnt);
    std::vector<float3> m_transmittance(m_cnt), m_emission(m_cnt);
    std::vector<float> m_shininess(m_cnt), m_ior(m_cnt), m_r_ior(m_cnt), m_dissolve(m_cnt);
    for (size_t i{0}; i < m_cnt; i++) {
        const auto& m = mats[i];
        m_ambient[i] = make_float3(m.ambient.x, m.ambient.y, m.ambient.z);
        m_diffuse[i] = make_float3(m.diffuse.x, m.diffuse.y, m.diffuse.z);
        m_specular[i] = make_float3(m.specular.x, m.specular.y, m.specular.z);
        m_transmittance[i] = make_float3(m.transmittance.x, m.transmittance.y, m.transmittance.z);
        m_emission[i] = make_float3(m.emission.x, m.emission.y, m.emission.z);
        m_shininess[i] = m.shininess;
        m_ior[i] = m.ior;
        m_r_ior[i] = m.r_ior;
        m_dissolve[i] = m.dissolve;
    }
    mat_ambient_.Upload(m_ambient);
    mat_diffuse_.Upload(m_diffuse);
    mat_specular_.Upload(m_specular);
    mat_transmittance_.Upload(m_transmittance);
    mat_emission_.Upload(m_emission);
    mat_shininess_.Upload(m_shininess);
    mat_ior_.Upload(m_ior);
    mat_r_ior_.Upload(m_r_ior);
    mat_dissolve_.Upload(m_dissolve);

    // Transfer area lights
    const auto& area_lights = scene.AreaLights();
    size_t al_cnt = area_lights.size();

    std::vector<float3> al_color(al_cnt);
    std::vector<int> al_tri_id(al_cnt);
    std::vector<float> al_surface_area(al_cnt);
    for (size_t i{0}; i < al_cnt; i++) {
        const auto& al = area_lights[i];
        al_color[i] = make_float3(al.color.x, al.color.y, al.color.z);
        al_tri_id[i] = al.triangle_id;
        al_surface_area[i] = al.surface_area;
    }
    al_color_.Upload(al_color);
    al_tri_id_.Upload(al_tri_id);
    al_surface_area_.Upload(al_surface_area);

    // Transfer point lights
    const auto& point_lights = scene.PointLights();
    size_t pl_cnt = point_lights.size();

    std::vector<float3> pl_pos(pl_cnt), pl_color(pl_cnt);
    for (size_t i{0}; i < pl_cnt; i++) {
        const auto& pl = point_lights[i];
        pl_pos[i] = make_float3(pl.pos.x, pl.pos.y, pl.pos.z);
        pl_color[i] = make_float3(pl.color.x, pl.color.y, pl.color.z);
    }
    pl_pos_.Upload(pl_pos);
    pl_color_.Upload(pl_color);
}

GpuSceneView GpuScene::GetView() const {
    return GpuSceneView{
        // Triangles
        tri_v0_pos_.Data(),
        tri_v1_pos_.Data(),
        tri_v2_pos_.Data(),
        tri_v0_norm_.Data(),
        tri_v1_norm_.Data(),
        tri_v2_norm_.Data(),
        tri_geom_norm_.Data(),
        tri_has_vn_.Data(),
        tri_mat_id_.Data(),
        static_cast<int>(tri_v0_pos_.Size()),

        // Materials
        mat_ambient_.Data(),
        mat_diffuse_.Data(),
        mat_specular_.Data(),
        mat_transmittance_.Data(),
        mat_emission_.Data(),
        mat_shininess_.Data(),
        mat_ior_.Data(),
        mat_r_ior_.Data(),
        mat_dissolve_.Data(),
        static_cast<int>(mat_ambient_.Size()),

        // PointLights
        pl_pos_.Data(),
        pl_color_.Data(),
        static_cast<int>(pl_pos_.Size()),

        // AreaLights
        al_color_.Data(),
        al_tri_id_.Data(),
        al_surface_area_.Data(),
        static_cast<int>(al_color_.Size()),
    };
}

}  // namespace diplodocus::cuda_kernels

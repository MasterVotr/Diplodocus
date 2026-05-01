#include "io/scene/obj_scene_loader.h"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <fstream>
#include <string>

// Third party includes
#include <rapidobj.hpp>

#include "scene/light.h"
#include "scene/material.h"
#include "scene/scene.h"
#include "scene/triangle.h"
#include "scene/vertex.h"
#include "util/logger.h"
#include "util/vec3.h"

namespace diplodocus {

namespace {

float EmissivePower(const rapidobj::Float3& emission) { return std::max({emission[0], emission[1], emission[2]}); }

Vec3 EmissiveColor(const rapidobj::Float3& emission, float power) {
    if (power <= 0.0f) return {0.0f, 0.0f, 0.0f};
    return {emission[0] / power, emission[1] / power, emission[2] / power};
}

Vec3 CalculateCameraUp(const Vec3& from, const Vec3& to, const Vec3& arbitrary_up) {
    Vec3 forward = Normalize(from - to);
    Vec3 right = Normalize(Cross(arbitrary_up, forward));
    return Cross(forward, right);
}

}  // namespace

bool ObjSceneLoader::LoadObj(std::filesystem::path obj_file_path, Scene& scene) {
    Logger::debug("ObjSceneLoader: Loading obj from {}", obj_file_path.native());

    rapidobj::Result result =
        rapidobj::ParseFile(obj_file_path, rapidobj::MaterialLibrary::Default(rapidobj::Load::Optional));

    if (result.error) {
        Logger::error("ObjSceneLoader: Error reading file {} using rapidobj, error:{} at line: {}: {}",
                      obj_file_path.native(), result.error.code.message(), result.error.line_num, result.error.line);
        return false;
    }

    bool success = rapidobj::Triangulate(result);

    if (!success) {
        Logger::error("ObjSceneLoader: Error triangulating file {} using rapidobj, error: {}", obj_file_path.native(),
                      result.error.code.message());
    }

    auto& attrib = result.attributes;
    auto& shapes = result.shapes;
    auto& materials = result.materials;

    // Reserve memory in scene
    size_t t_cnt = 0;
    for (const auto& shape : shapes) t_cnt += shape.mesh.num_face_vertices.size();
    scene.ReserveTriangles(t_cnt);
    scene.ReserverMaterials(materials.size());

    // Loop over shapes
    for (const auto& shape : shapes) {
        bool warned_about_vertex_normals = false;
        // Loop per faces
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            if (shape.mesh.num_face_vertices[f] != 3) {
                Logger::error("ObjSceneLoader: Cannot load other shapes that triangles");
                return false;
            }
            Triangle t;
            auto idx0 = shape.mesh.indices[3 * f + 0];
            auto idx1 = shape.mesh.indices[3 * f + 1];
            auto idx2 = shape.mesh.indices[3 * f + 2];

            // Vertex positions
            t.v0.pos = {attrib.positions[3 * idx0.position_index + 0], attrib.positions[3 * idx0.position_index + 1],
                        attrib.positions[3 * idx0.position_index + 2]};
            t.v1.pos = {attrib.positions[3 * idx1.position_index + 0], attrib.positions[3 * idx1.position_index + 1],
                        attrib.positions[3 * idx1.position_index + 2]};
            t.v2.pos = {attrib.positions[3 * idx2.position_index + 0], attrib.positions[3 * idx2.position_index + 1],
                        attrib.positions[3 * idx2.position_index + 2]};

            // Check if `normal_index` is zero or positive. negative = no normal data
            if (idx0.normal_index >= 0 && idx1.normal_index >= 0 && idx2.normal_index >= 0) {
                t.v0.normal = {attrib.normals[3 * idx0.normal_index + 0], attrib.normals[3 * idx0.normal_index + 1],
                               attrib.normals[3 * idx0.normal_index + 2]};
                t.v1.normal = {attrib.normals[3 * idx1.normal_index + 0], attrib.normals[3 * idx1.normal_index + 1],
                               attrib.normals[3 * idx1.normal_index + 2]};
                t.v2.normal = {attrib.normals[3 * idx2.normal_index + 0], attrib.normals[3 * idx2.normal_index + 1],
                               attrib.normals[3 * idx2.normal_index + 2]};
                t.has_vertex_normals = true;
            } else {
                if (!warned_about_vertex_normals) {
                    warned_about_vertex_normals = true;
                    Logger::debug(
                        "ObjSceneLoader: Vertex normals not present in shape {}, only flat shading will be available",
                        shape.name);
                }
                t.has_vertex_normals = false;
            }
            t.material_id = shape.mesh.material_ids[f];
            if (t.material_id < 0) {
                Logger::info("ObjLoader: Triangle {} does not have a material (default = -1)",
                             scene.Triangles().size());
            }
            scene.AddTriangle(t);

            // Add area light if triangle is emissive
            if (t.material_id >= 0) {
                const auto& material = materials[t.material_id];
                if (material.emission[0] != 0.0f || material.emission[1] != 0.0f || material.emission[2] != 0.0f) {
                    AreaLight al;
                    al.triangle_id = scene.Triangles().size() - 1;
                    al.power = EmissivePower(material.emission);
                    al.color = EmissiveColor(material.emission, al.power);
                    al.surface_area = CalculateTriangleSurfaceArea(t);
                    scene.AddAreaLight(al);
                }
            }
        }
    }

    // Loop over materials
    for (const auto& material : materials) {
        Material m;
        m.name = material.name;
        m.ambient = {material.ambient[0], material.ambient[1], material.ambient[2]};
        m.diffuse = {material.diffuse[0], material.diffuse[1], material.diffuse[2]};
        m.specular = {material.specular[0], material.specular[1], material.specular[2]};
        m.transmittance = {material.transmittance[0], material.transmittance[1], material.transmittance[2]};
        m.emission = {material.emission[0], material.emission[1], material.emission[2]};
        m.shininess = material.shininess;
        m.ior = material.ior;
        m.r_ior = 1.0f / material.ior;
        m.dissolve = material.dissolve;
        scene.AddMaterial(m);
    }

    return true;
}

bool ObjSceneLoader::LoadMetadata(std::filesystem::path metadata_file_path, Scene& scene) {
    Logger::debug("ObjSceneLoader: Loading metadata from {}", metadata_file_path.native());
    std::ifstream input(metadata_file_path);
    if (!input) {
        Logger::error("ObjSceneLoader: Error reading metadata file {}", metadata_file_path.native());
        return false;
    }

    auto& camera = scene.GetCamera();
    std::string token;
    while (input >> token) {
        if (token == "cam_pos") {
            if (!(input >> camera.pos.x >> camera.pos.y >> camera.pos.z)) {
                Logger::debug("ObjSceneLoader: Could not load camera position metadata");
                return false;
            }
        } else if (token == "cam_up") {
            if (!(input >> camera.up.x >> camera.up.y >> camera.up.z)) {
                Logger::debug("ObjSceneLoader: Could not load camera up vector metadata");
                return false;
            }
        } else if (token == "cam_dir") {
            if (!(input >> camera.dir.x >> camera.dir.y >> camera.dir.z)) {
                Logger::debug("ObjSceneLoader: Could not load camera direction metadata");
                return false;
            }
        } else if (token == "cam_fov") {
            if (!(input >> camera.fov)) {
                Logger::debug("ObjSceneLoader: Could not load camera field of view metadata");
                return false;
            }
        } else if (token == "cam_far") {
            if (!(input >> camera.far)) {
                Logger::debug("ObjSceneLoader: Could not load camera far plane metadata");
                return false;
            }
        } else if (token == "point_light") {
            PointLight pl;
            if (!(input >> pl.pos.x >> pl.pos.y >> pl.pos.z)) {
                Logger::debug("ObjSceneLoader: Could not load point light position metadata");
                return false;
            }
            if (!(input >> pl.color.x >> pl.color.y >> pl.color.z)) {
                Logger::debug("ObjSceneLoader: Could not load point light color metadata");
                return false;
            }
            if (!(input >> pl.power)) {
                Logger::debug("ObjSceneLoader: Could not load point light power metadata");
                return false;
            }
            scene.AddPointLight(pl);
        } else {
            Logger::error("ObjSceneLoader: Unrecognized metadata token '{}'", token);
            return false;
        }
    }

    camera.dir = Normalize(camera.dir);
    camera.up = CalculateCameraUp(camera.pos, camera.pos + camera.dir, camera.up);

    return true;
}

std::optional<Scene> ObjSceneLoader::Load(const SceneLoadConfig& config) const {
    Logger::info("ObjSceneLoader: Loading {}...", config.name);
    std::filesystem::path basepath = config.dirpath;

    std::filesystem::path metadata_file_path = basepath / config.meta_data;
    metadata_file_path += ".meta";

    std::filesystem::path obj_file_path = basepath / config.name;
    obj_file_path += ".obj";

    Scene scene;
    try {
        if (!LoadMetadata(metadata_file_path, scene)) return {};
        if (!LoadObj(obj_file_path, scene)) return {};
    } catch (std::exception e) {
        Logger::error("ObjSceneLoder: Failed to parse {} and/or {}: {}", metadata_file_path.c_str(),
                      obj_file_path.native(), e.what());
    }

    return scene;
}

}  // namespace diplodocus

#include "io/config/json_config_loader.h"

#include <exception>
#include <fstream>
#include <string>

// Third party includes
#include <json.hpp>
#include <type_traits>

#include "config/acceleration_structure_config.h"
#include "config/config.h"
#include "config/image_export_config.h"
#include "config/render_config.h"
#include "config/scene_load_config.h"
#include "config/stats_export_config.h"
#include "util/logger.h"
#include "util/util.h"
#include "util/vec3.h"

namespace diplodocus {

template <typename T>
void SetIfExists(const nlohmann::json& json_config, const char* key, T& value) {
    auto it = json_config.find(key);
    if (it == json_config.end() || it->is_null()) return;
    it->get_to(value);
}

// Overloaded function from nlohmann::json to parse a Vec3s
void from_json(const nlohmann::json& j, Vec3& v) {
    v.x = j.at(0);
    v.y = j.at(1);
    v.z = j.at(2);
}

// Overloaded function from nlohmann::json to parse an Enum
template <typename Enum>
    requires std::is_enum_v<Enum>
void from_json(const nlohmann::json& j, Enum& e) {
    e = ParseEnum<Enum>(j.get<std::string>());
}

std::optional<Config> JsonConfigLoader::Load(std::filesystem::path config_file_path) const {
    std::ifstream input(config_file_path);
    if (!input) {
        Logger::error("JsonConfigLoader: Could not open file '{}'", config_file_path.native());
        return {};
    }

    // Load config
    Config config;
    try {
        const nlohmann::json json_config = nlohmann::json::parse(input);

        // Image export config
        if (json_config.contains("image_export"))
            config.image_export_config = LoadImageExportConfig(json_config.at("image_export"));

        // Render config
        if (json_config.contains("render")) config.render_config = LoadRenderConfig(json_config.at("render"));

        // Scene load config
        if (json_config.contains("scene_load"))
            config.scene_load_config = LoadSceneLoadConfig(json_config.at("scene_load"));

        // Stats export config
        if (json_config.contains("stats_export"))
            config.stats_export_config = LoadStatsExportConfig(json_config.at("stats_export"));

        // Acceleration structure conig
        if (json_config.contains("acceleration_structure"))
            config.acceleration_structure_config =
                LoadAccelerationStructureConfig(json_config.at("acceleration_structure"));

    } catch (const std::exception e) {
        Logger::error("JsonConfigLoader: Failed to parse '{}': {}", config_file_path.native(), e.what());
        return {};
    }

    return config;
}

ImageExportConfig JsonConfigLoader::LoadImageExportConfig(nlohmann::json json_config) {
    ImageExportConfig image_export_config;

    SetIfExists(json_config, "image_export_format", image_export_config.image_export_format);
    SetIfExists(json_config, "filepath", image_export_config.filepath);

    return image_export_config;
}

RenderConfig JsonConfigLoader::LoadRenderConfig(nlohmann::json json_config) {
    RenderConfig render_config;

    SetIfExists(json_config, "renderer_type", render_config.renderer_type);
    SetIfExists(json_config, "width", render_config.width);
    SetIfExists(json_config, "height", render_config.height);
    SetIfExists(json_config, "background_color", render_config.background_color);
    SetIfExists(json_config, "backface_culling", render_config.backface_culling);
    SetIfExists(json_config, "max_depth", render_config.max_depth);
    SetIfExists(json_config, "area_light_sample_cnt", render_config.area_light_sample_cnt);
    SetIfExists(json_config, "pixel_sample_cnt", render_config.pixel_sample_cnt);
    SetIfExists(json_config, "seed", render_config.seed);

    return render_config;
}

SceneLoadConfig JsonConfigLoader::LoadSceneLoadConfig(nlohmann::json json_config) {
    SceneLoadConfig scene_load_config;

    SetIfExists(json_config, "dirpath", scene_load_config.dirpath);
    SetIfExists(json_config, "name", scene_load_config.name);
    SetIfExists(json_config, "triangulate", scene_load_config.triangulate);
    SetIfExists(json_config, "scene_load_format", scene_load_config.scene_load_format);

    return scene_load_config;
}

StatsExportConfig JsonConfigLoader::LoadStatsExportConfig(nlohmann::json json_config) {
    StatsExportConfig stats_export_config;

    SetIfExists(json_config, "stats_export_format", stats_export_config.stats_export_format);

    return stats_export_config;
}

AccelerationStructureConfig JsonConfigLoader::LoadAccelerationStructureConfig(nlohmann::json json_config) {
    AccelerationStructureConfig acceleration_structure_config;

    SetIfExists(json_config, "acceleration_structure_type", acceleration_structure_config.acceleration_structure_type);
    SetIfExists(json_config, "max_depth", acceleration_structure_config.max_depth);
    SetIfExists(json_config, "max_triangles_in_leaf", acceleration_structure_config.max_triangles_in_leaf);
    SetIfExists(json_config, "nn_search_radius", acceleration_structure_config.nn_search_radius);
    SetIfExists(json_config, "kdop_size", acceleration_structure_config.kdop_size);
    SetIfExists(json_config, "max_triangles_per_BB", acceleration_structure_config.max_triangles_per_BB);
    SetIfExists(json_config, "bin_count", acceleration_structure_config.bin_count);

    return acceleration_structure_config;
}

}  // namespace diplodocus

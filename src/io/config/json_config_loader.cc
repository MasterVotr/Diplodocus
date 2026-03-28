#include "io/config/json_config_loader.h"

#include <exception>
#include <fstream>
#include <string>

// Third party includes
#include <json.hpp>

#include "config/config.h"
#include "config/image_export_config.h"
#include "config/render_config.h"
#include "config/scene_load_config.h"
#include "config/stats_export_config.h"
#include "util/logger.h"

namespace diplodocus {

std::optional<Config> JsonConfigLoader::Load(std::filesystem::path config_file_path) const {
    std::ifstream input(config_file_path);
    if (!input) {
        Logger::error("JsonConfigLoader: Could not open file '{}'", config_file_path.native());
        return {};
    }

    // Load config
    Config config;
    try {
        nlohmann::json json_config = nlohmann::json::parse(input);

        config.image_export_config = LoadImageExportConfig(json_config.at("image_export"));
        config.render_config = LoadRenderConfig(json_config.at("render"));
        config.scene_load_config = LoadSceneLoadConfig(json_config.at("scene_load"));
        config.stats_export_config = LoadStatsExportConfig(json_config.at("stats_export"));
    } catch (const std::exception e) {
        Logger::error("JsonConfigLoader: Failed to parse '{}': {}", config_file_path.native(), e.what());
        return {};
    }

    return config;
}

ImageExportConfig JsonConfigLoader::LoadImageExportConfig(nlohmann::json json_config) {
    ImageExportConfig image_export_config;

    image_export_config.filepath = json_config.at("filepath").get<std::string>();

    // Image export format
    if (json_config.at("image_export_format") == "ppm")
        image_export_config.image_export_format = ImageExportFormat::kPpm;
    else
        Logger::error("JsonConfigLoader: Unknown image export format");

    return image_export_config;
}

RenderConfig JsonConfigLoader::LoadRenderConfig(nlohmann::json json_config) {
    RenderConfig render_config;

    render_config.width = json_config.at("width");
    render_config.height = json_config.at("height");
    render_config.background_color.x = json_config.at("background_color")[0];
    render_config.background_color.y = json_config.at("background_color")[1];
    render_config.background_color.z = json_config.at("background_color")[2];
    render_config.backface_culling = json_config.at("backface_culling");
    render_config.max_depth = json_config.at("max_depth");
    render_config.area_light_sample_cnt = json_config.at("area_light_sample_cnt");

    return render_config;
}

SceneLoadConfig JsonConfigLoader::LoadSceneLoadConfig(nlohmann::json json_config) {
    SceneLoadConfig scene_load_config;

    scene_load_config.dirpath = json_config.at("dirpath").get<std::string>();
    scene_load_config.name = json_config.at("name").get<std::string>();
    scene_load_config.triangulate = json_config.at("triangulate").get<bool>();

    // Scene load format
    if (json_config.at("scene_load_format") == "obj")
        scene_load_config.scene_load_format = SceneLoadFormat::kObj;
    else
        Logger::error("JsonConfigLoader: Unkonw scene load format");

    return scene_load_config;
}

StatsExportConfig JsonConfigLoader::LoadStatsExportConfig(nlohmann::json json_config) {
    StatsExportConfig stats_export_config;

    // Stats export format
    if (json_config.at("stats_export_format") == "console")
        stats_export_config.stats_export_format = StatsExportFormat::kConsole;
    else if (json_config.at("stats_export_format") == "csv")
        stats_export_config.stats_export_format = StatsExportFormat::kCsv;
    else
        Logger::error("JsonConfigLoader: Unknown stats export format");

    return stats_export_config;
}

}  // namespace diplodocus

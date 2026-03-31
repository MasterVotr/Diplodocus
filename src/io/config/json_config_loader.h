#pragma once

#include <json.hpp>

#include "config/acceleration_structure_config.h"
#include "config/image_export_config.h"
#include "config/render_config.h"
#include "config/scene_load_config.h"
#include "io/config/config_loader.h"

namespace diplodocus {

class JsonConfigLoader : public ConfigLoader {
   public:
    std::optional<Config> Load(std::filesystem::path config_file_path) const override;

   private:
    static ImageExportConfig LoadImageExportConfig(nlohmann::json json_config);
    static RenderConfig LoadRenderConfig(nlohmann::json json_config);
    static SceneLoadConfig LoadSceneLoadConfig(nlohmann::json json_config);
    static StatsExportConfig LoadStatsExportConfig(nlohmann::json json_config);
    static AccelerationStructureConfig LoadAccelerationStructureConfig(nlohmann::json json_config);
};

}  // namespace diplodocus

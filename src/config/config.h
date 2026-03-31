#pragma once

#include "config/acceleration_structure_config.h"
#include "config/image_export_config.h"
#include "config/render_config.h"
#include "config/scene_load_config.h"
#include "config/stats_export_config.h"

namespace diplodocus {

struct Config {
    AccelerationStructureConfig acceleration_structure_config;
    ImageExportConfig image_export_config;
    RenderConfig render_config;
    SceneLoadConfig scene_load_config;
    StatsExportConfig stats_export_config;
};

}  // namespace diplodocus

#include "app/cli_app.h"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <memory>

#include "config/render_config.h"
#include "io/config/json_config_loader.h"
#include "io/image/image_exporter_factory.h"
#include "io/scene/scene_loader_factory.h"
#include "io/stats/stats_exporter_factory.h"
#include "renderer/cpu_raytracer.h"
#include "renderer/gpu_raytracer.h"
#include "util/logger.h"

namespace diplodocus {

CliApp::CliApp(AppParameters app_params) : App(app_params) {
    Logger::info("CliApp created");

    // Parse console arguments
    if (app_params_.app_console_args.count != 2) {
        Logger::error("Usage: ./app <config.json>");
        exit(EXIT_FAILURE);
    }
    std::filesystem::path config_path(app_params.app_console_args[1]);

    // Setup config
    auto config_loader_ = std::make_unique<JsonConfigLoader>();
    app_ctx_.config = config_loader_->Load(config_path).value();

    // Setup Renderers
    renderers_.insert({RendererType::kCpu, std::make_unique<CpuRaytracer>()});
    renderers_.insert({RendererType::kGpu, std::make_unique<GpuRaytracer>()});
}

void CliApp::Run() {
    // Load scene
    auto scene_loader = CreateSceneLoader(app_ctx_.config.scene_load_config);
    app_ctx_.scene = std::move(scene_loader->Load(app_ctx_.config.scene_load_config).value());

    // Render image
    renderers_[app_ctx_.config.render_config.renderer_type]->StartRender(
        app_ctx_.config.render_config, app_ctx_.config.acceleration_structure_config, app_ctx_.scene,
        app_ctx_.framebuffer, app_ctx_.stats);

    // Export image
    auto image_exporter = CreateImageExporter(app_ctx_.config.image_export_config);
    image_exporter->Export(app_ctx_.config.image_export_config, app_ctx_.framebuffer);

    // Export stats
    auto stats_exporter = CreateStatsExporter(app_ctx_.config.stats_export_config);
    stats_exporter->ExportOne(app_ctx_.config.stats_export_config, app_ctx_.stats);
}

}  // namespace diplodocus

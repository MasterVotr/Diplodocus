#pragma once

#include <filesystem>
#include <map>

#include "app/app.h"
#include "config/render_config.h"
#include "renderer/renderer.h"

namespace diplodocus {

class CliApp : public App {
   public:
    CliApp(AppParameters app_params);

    void Run() override;

   private:
    std::map<RendererType, std::unique_ptr<Renderer>> renderers_;

    void SetupConfig(std::filesystem::path config_path);
    void SetupSystems();
};

}  // namespace diplodocus

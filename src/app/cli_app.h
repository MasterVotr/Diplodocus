#pragma once

#include <filesystem>

#include "app/app.h"
#include "renderer/renderer.h"

namespace diplodocus {

class CliApp : public App {
   public:
    CliApp(AppParameters app_params);

    void Run() override;

   private:
    std::unique_ptr<Renderer> renderer_;

    void SetupConfig(std::filesystem::path config_path);
    void SetupSystems();
};

}  // namespace diplodocus

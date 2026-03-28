#pragma once

#include <memory>

#include "config/scene_load_config.h"
#include "io/scene/scene_loader.h"

namespace diplodocus {

std::unique_ptr<SceneLoader> CreateSceneLoader(const SceneLoadConfig& scene_load_config);

}  // namespace diplodocus

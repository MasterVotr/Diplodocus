#pragma once

#include <optional>

#include "config/scene_load_config.h"
#include "scene/scene.h"

namespace diplodocus {

class SceneLoader {
   public:
    virtual ~SceneLoader() = default;
    virtual std::optional<Scene> Load(const SceneLoadConfig& config) const = 0;
};

}  // namespace diplodocus

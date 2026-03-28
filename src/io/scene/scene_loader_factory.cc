#include "io/scene/scene_loader_factory.h"

#include <memory>
#include <stdexcept>

#include "config/scene_load_config.h"
#include "io/scene/obj_scene_loader.h"
#include "io/scene/scene_loader.h"

namespace diplodocus {

std::unique_ptr<SceneLoader> CreateSceneLoader(const SceneLoadConfig& scene_load_config) {
    switch (scene_load_config.scene_load_format) {
        case SceneLoadFormat::kObj:
            return std::make_unique<ObjSceneLoader>();
        default:
            throw std::invalid_argument("Unkown SceneLoadFormat");
    }
}

}  // namespace diplodocus

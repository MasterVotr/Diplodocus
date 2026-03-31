#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>

#include "util/util.h"

namespace diplodocus {

enum class SceneLoadFormat { kObj };

template <>
inline SceneLoadFormat ParseEnum<SceneLoadFormat>(std::string_view sv) {
    if (sv == "obj") return SceneLoadFormat::kObj;
    throw std::runtime_error("Invalid SceneLoadFormat: " + std::string(sv));
}

struct SceneLoadConfig {
    SceneLoadFormat scene_load_format = SceneLoadFormat::kObj;
    std::filesystem::path dirpath = "res";
    std::string name = "CornellBox-Sphere";
    bool triangulate = true;
};

}  // namespace diplodocus

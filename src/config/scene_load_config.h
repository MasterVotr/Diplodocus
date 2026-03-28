#pragma once

#include <filesystem>
#include <string>

namespace diplodocus {

enum class SceneLoadFormat { kObj };

struct SceneLoadConfig {
    SceneLoadFormat scene_load_format;
    std::filesystem::path dirpath;
    std::string name;
    bool triangulate;
};

}  // namespace diplodocus

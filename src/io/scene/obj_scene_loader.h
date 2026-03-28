#pragma once

#include "io/scene/scene_loader.h"

namespace diplodocus {

class ObjSceneLoader : public SceneLoader {
   public:
    std::optional<Scene> Load(const SceneLoadConfig& config) const override;

   private:
    static bool LoadObj(std::filesystem::path obj_file_path, Scene& scene);
    static bool LoadMetadata(std::filesystem::path metadata_file_path, Scene& scene);
};

}  // namespace diplodocus

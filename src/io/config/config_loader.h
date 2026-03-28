#pragma once

#include <filesystem>
#include <optional>

#include "config/config.h"

namespace diplodocus {

class ConfigLoader {
   public:
    virtual ~ConfigLoader() = default;
    virtual std::optional<Config> Load(std::filesystem::path config_file_path) const = 0;
};

}  // namespace diplodocus

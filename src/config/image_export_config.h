#pragma once

#include <filesystem>
#include <stdexcept>
#include <string_view>

#include "util/util.h"

namespace diplodocus {

enum class ImageExportFormat { kPpm };

template <>
inline ImageExportFormat ParseEnum<ImageExportFormat>(std::string_view sv) {
    if (sv == "ppm") return ImageExportFormat::kPpm;
    throw std::runtime_error("Invalid ImageExportFormat: " + std::string(sv));
}

struct ImageExportConfig {
    ImageExportFormat image_export_format = ImageExportFormat::kPpm;
    std::filesystem::path filepath = "output.ppm";
};

}  // namespace diplodocus

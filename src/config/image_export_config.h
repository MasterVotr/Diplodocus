#pragma once

#include <filesystem>

namespace diplodocus {

enum class ImageExportFormat { kPpm };

struct ImageExportConfig {
    ImageExportFormat image_export_format;
    std::filesystem::path filepath;
};

}  // namespace diplodocus

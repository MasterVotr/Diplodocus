#pragma once

#include <memory>

#include "config/image_export_config.h"
#include "io/image/image_exporter.h"

namespace diplodocus {

std::unique_ptr<ImageExporter> CreateImageExporter(const ImageExportConfig& config);

}  // namespace diplodocus

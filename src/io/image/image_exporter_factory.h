#pragma once

#include <memory>

#include "config/image_exporter_config.h"
#include "io/image/image_exporter.h"

namespace diplodocus {

std::unique_ptr<ImageExporter> CreateImageExporter(const ImageExporterConfig& config);

}  // namespace diplodocus

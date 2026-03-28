#include "io/image/image_exporter_factory.h"

#include <memory>
#include <stdexcept>

#include "config/image_export_config.h"
#include "io/image/ppm_image_exporter.h"

namespace diplodocus {

std::unique_ptr<ImageExporter> CreateImageExporter(const ImageExportConfig& config) {
    switch (config.image_export_format) {
        case ImageExportFormat::kPpm:
            return std::make_unique<PpmImageExporter>();
        default:
            throw std::invalid_argument("Unknown ImageExportFormat");
    }
}

}  // namespace diplodocus

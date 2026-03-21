#include "io/image/image_exporter_factory.h"

#include <memory>
#include <stdexcept>

#include "config/image_exporter_config.h"
#include "io/image/ppm_image_exporter.h"

namespace diplodocus {

std::unique_ptr<ImageExporter> CreateImageExporter(const ImageExporterConfig& config) {
    switch (config.image_exporter_type) {
        case ImageExporterType::kPpm:
            return std::make_unique<PpmImageExporter>();
        default:
            throw std::runtime_error("Unknown image exporter type");
    }
}

}  // namespace diplodocus

#pragma once

#include "config/image_exporter_config.h"
#include "framebuffer/framebuffer.h"

namespace diplodocus {

class ImageExporter {
   public:
    virtual ~ImageExporter() = default;
    virtual void Export(const ImageExporterConfig& config, const Framebuffer& framebuffer) const = 0;
};

}  // namespace diplodocus

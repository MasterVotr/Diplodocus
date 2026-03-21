#pragma once

#include "io/image/image_exporter.h"

namespace diplodocus {

class PpmImageExporter : public ImageExporter {
   public:
    virtual void Export(const ImageExporterConfig& config, const Framebuffer& framebuffer) const override;
};

}  // namespace diplodocus

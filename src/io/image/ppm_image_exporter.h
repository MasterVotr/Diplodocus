#pragma once

#include "io/image/image_exporter.h"

namespace diplodocus {

class PpmImageExporter : public ImageExporter {
   public:
    void Export(const ImageExportConfig& config, const Framebuffer& framebuffer) const override;
};

}  // namespace diplodocus

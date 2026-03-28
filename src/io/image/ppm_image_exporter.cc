#include "io/image/ppm_image_exporter.h"

#include <algorithm>
#include <fstream>

#include "util/logger.h"

namespace diplodocus {

void PpmImageExporter::Export(const ImageExportConfig& config, const Framebuffer& framebuffer) const {
    const int w = framebuffer.GetWidth();
    const int h = framebuffer.GetHeight();
    const auto& data = framebuffer.GetData();

    std::ofstream output(config.filepath);
    if (!output) {
        Logger::error("PPM Image exporter: Could not write to file {}!", config.filepath.native());
        return;
    }

    const auto to_u8 = [](float v) { return static_cast<int>(255.999f * std::clamp(v, 0.0f, 1.0f)); };

    output << "P3\n" << framebuffer.GetWidth() << ' ' << framebuffer.GetHeight() << "\n255\n";
    for (int i = 0; i < w * h; i++) {
        output << to_u8(data[i * 3 + 0]) << ' ';
        output << to_u8(data[i * 3 + 1]) << ' ';
        output << to_u8(data[i * 3 + 2]) << '\n';
    }

    output.close();
    Logger::info("Exported framebuffer to {}", config.filepath.native());
}

}  // namespace diplodocus

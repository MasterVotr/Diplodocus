#include "io/image/ppm_image_exporter.h"

#include <fstream>

#include "util/logger.h"

namespace diplodocus {

void PpmImageExporter::Export(const ImageExporterConfig& config, const Framebuffer& framebuffer) const {
    int w = framebuffer.GetWidth();
    int h = framebuffer.GetHeight();
    const auto& data = framebuffer.GetData();
    std::ofstream output(config.filepath.c_str());

    output << "P3\n" << framebuffer.GetWidth() << ' ' << framebuffer.GetHeight() << "\n255\n";
    for (int i = 0; i < w * h; i++) {
        output << static_cast<int>(255.999 * (data[i * 3 + 0])) << ' ';
        output << static_cast<int>(255.999 * (data[i * 3 + 1])) << ' ';
        output << static_cast<int>(255.999 * (data[i * 3 + 2])) << '\n';
    }

    output.close();
    Logger::info("Exported framebuffer to {}", config.filepath.c_str());
}

}  // namespace diplodocus

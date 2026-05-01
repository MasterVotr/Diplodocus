#pragma once

#include <filesystem>
#include <stdexcept>

#include "util/util.h"

namespace diplodocus {

enum class StatsExportFormat { kConsole, kJson };

template <>
inline StatsExportFormat ParseEnum<StatsExportFormat>(std::string_view sv) {
    if (sv == "console") return StatsExportFormat::kConsole;
    if (sv == "json") return StatsExportFormat::kJson;
    throw std::runtime_error("Invalid StatsExportFormat: " + std::string(sv));
}

struct StatsExportConfig {
    StatsExportFormat stats_export_format = StatsExportFormat::kConsole;
    std::filesystem::path filepath = "output.json";
};

}  // namespace diplodocus

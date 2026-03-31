#pragma once

#include <stdexcept>

#include "util/util.h"

namespace diplodocus {

enum class StatsExportFormat { kConsole, kCsv };

template <>
inline StatsExportFormat ParseEnum<StatsExportFormat>(std::string_view sv) {
    if (sv == "console") return StatsExportFormat::kConsole;
    if (sv == "csv") return StatsExportFormat::kCsv;
    throw std::runtime_error("Invalid StatsExportFormat: " + std::string(sv));
}

struct StatsExportConfig {
    StatsExportFormat stats_export_format = StatsExportFormat::kConsole;
};

}  // namespace diplodocus

#pragma once

namespace diplodocus {

enum class StatsExportFormat { kConsole, kCsv };

struct StatsExportConfig {
    StatsExportFormat stats_export_format = StatsExportFormat::kConsole;
};

}  // namespace diplodocus

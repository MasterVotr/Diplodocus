#include "io/stats/stats_exporter_factory.h"

#include <memory>
#include <stdexcept>

#include "config/stats_export_config.h"
#include "io/stats/console_stats_exporter.h"
#include "io/stats/json_stats_exporter.h"

namespace diplodocus {

std::unique_ptr<StatsExporter> CreateStatsExporter(const StatsExportConfig& stats_export_config) {
    switch (stats_export_config.stats_export_format) {
        case StatsExportFormat::kConsole:
            return std::make_unique<ConsoleStatsExporter>();
        case StatsExportFormat::kJson:
            return std::make_unique<JsonStatsExporter>();
        default:
            throw std::invalid_argument("Unknown StatExportFormat");
    }
}

}  // namespace diplodocus

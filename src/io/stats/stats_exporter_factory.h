#pragma once

#include <memory>

#include "config/stats_export_config.h"
#include "io/stats/stats_exporter.h"

namespace diplodocus {

std::unique_ptr<StatsExporter> CreateStatsExporter(const StatsExportConfig& stats_export_config);

}  // namespace diplodocus

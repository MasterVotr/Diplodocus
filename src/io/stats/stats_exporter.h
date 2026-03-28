#pragma once

// #include <vector>

#include "config/stats_export_config.h"
#include "stats/stats.h"

namespace diplodocus {

class StatsExporter {
   public:
    virtual ~StatsExporter() = default;
    virtual void ExportOne(const StatsExportConfig& stats_export_config, const Stats& stats) = 0;
    // virtual void ExportAll(const StatsExportConfig& stats_export_config, std::vector<Stats>& stats_all) = 0;
};

}  // namespace diplodocus

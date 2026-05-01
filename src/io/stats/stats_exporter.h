#pragma once

// #include <vector>

#include "config/config.h"
#include "stats/stats.h"

namespace diplodocus {

class StatsExporter {
   public:
    virtual ~StatsExporter() = default;
    virtual void ExportOne(const Config& config, const Stats& stats) = 0;
    // virtual void ExportAll(const StatsExportConfig& stats_export_config, std::vector<Stats>& stats_all) = 0;
};

}  // namespace diplodocus

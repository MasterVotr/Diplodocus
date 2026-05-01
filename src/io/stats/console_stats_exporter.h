#pragma once

#include "io/stats/stats_exporter.h"
#include "stats/construction_stats.h"
#include "stats/raytracing_stats.h"

namespace diplodocus {

class ConsoleStatsExporter : public StatsExporter {
   public:
    void ExportOne(const Config& config, const Stats& stats) override;
    // void ExportAll(const StatsExportConfig& stats_export_config, std::vector<Stats>& stats_all) override;
   private:
    static void ExportConstructionStats(const ConstructionStats& accel_stats);
    static void ExportRaytracingStats(const RaytracingStats& rt_stats);
};

}  // namespace diplodocus

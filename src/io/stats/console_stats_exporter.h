#pragma once

// #include <vector>

#include "io/stats/stats_exporter.h"
#include "stats/acceleration_stats.h"
#include "stats/raytracing_stats.h"

namespace diplodocus {

class ConsoleStatsExporter : public StatsExporter {
   public:
    void ExportOne(const StatsExportConfig& stats_export_config, const Stats& stats) override;
    // void ExportAll(const StatsExportConfig& stats_export_config, std::vector<Stats>& stats_all) override;
   private:
    static void ExportRaytracingStats(const RaytracingStats& rt_stats);
    static void ExportAccelerationStats(const AccelerationStats& accel_stats);
};

}  // namespace diplodocus

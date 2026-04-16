#include "io/stats/console_stats_exporter.h"

#include <iostream>
#include <ostream>

#include "util/util.h"

namespace diplodocus {

void ConsoleStatsExporter::ExportOne([[maybe_unused]] const StatsExportConfig& stats_export_config,
                                     const Stats& stats) {
    PrintLnFmt(std::cout, "########################################");
    PrintLnFmt(std::cout, "#                                      #");
    PrintLnFmt(std::cout, "#              Statistics              #");
    PrintLnFmt(std::cout, "#                                      #");
    PrintLnFmt(std::cout, "########################################");
    PrintLnFmt(std::cout, "");
    ExportRaytracingStats(stats.rt_stats);
    PrintLnFmt(std::cout, "");
    ExportAccelerationStats(stats.accel_stats);
    PrintLnFmt(std::cout, "");
    PrintLnFmt(std::cout, "########################################");
    std::cout.flush();
}

void ConsoleStatsExporter::ExportRaytracingStats(const RaytracingStats& rt_stats) {
    PrintLnFmt(std::cout, "----------------------------------------");
    PrintLnFmt(std::cout, "               Ray tracing              ");
    PrintLnFmt(std::cout, "----------------------------------------");
    PrintLnFmt(std::cout, " Ray tracing time:    {:.3} s", rt_stats.raytracing_time);
    PrintLnFmt(std::cout, " Primary ray count:   {}", rt_stats.primary_ray_count);
    PrintLnFmt(std::cout, " Secondary ray count: {}", rt_stats.secondary_ray_count);
    PrintLnFmt(std::cout, " Shadow ray count:    {}", rt_stats.shadow_ray_count);
}

void ConsoleStatsExporter::ExportAccelerationStats(const AccelerationStats& accel_stats) {
    PrintLnFmt(std::cout, "----------------------------------------");
    PrintLnFmt(std::cout, "               Acceleration             ");
    PrintLnFmt(std::cout, "----------------------------------------");
    PrintLnFmt(std::cout, "# Construction:                         ");
    PrintLnFmt(std::cout, " Build time:          {:.3} ms", accel_stats.build_time);
    PrintLnFmt(std::cout, " Node count:          {}", accel_stats.node_count);
    PrintLnFmt(std::cout, " Inner node count:    {}", accel_stats.inner_node_count);
    PrintLnFmt(std::cout, " Leaf node count:     {}", accel_stats.leaf_node_count);
    PrintLnFmt(std::cout, " Memory consumption:  {:.3} MB", accel_stats.memory_consumption / 1e6f);
    PrintLnFmt(std::cout, "");
    PrintLnFmt(std::cout, "# Query:                                 ");
    PrintLnFmt(std::cout, " Query count:         {}", accel_stats.query_count);
    PrintLnFmt(std::cout, " Intersection count:  {}", accel_stats.intersection_count);
    PrintLnFmt(std::cout, " Traversal count:     {}", accel_stats.traversal_count);
}

}  // namespace diplodocus

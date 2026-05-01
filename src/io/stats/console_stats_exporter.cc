#include "io/stats/console_stats_exporter.h"

#include <cstdint>
#include <iostream>
#include <ostream>

#include "util/util.h"

namespace diplodocus {

void ConsoleStatsExporter::ExportOne([[maybe_unused]] const Config& config, const Stats& stats) {
    PrintLnFmt(std::cout, "########################################");
    PrintLnFmt(std::cout, "#                                      #");
    PrintLnFmt(std::cout, "#              Statistics              #");
    PrintLnFmt(std::cout, "#                                      #");
    PrintLnFmt(std::cout, "########################################");
    PrintLnFmt(std::cout, "");
    ExportConstructionStats(stats.construction_stats);
    PrintLnFmt(std::cout, "");
    ExportRaytracingStats(stats.rt_stats);
    PrintLnFmt(std::cout, "");
    PrintLnFmt(std::cout, "########################################");
    std::cout.flush();
}

void ConsoleStatsExporter::ExportConstructionStats(const ConstructionStats& construction_stats) {
    PrintLnFmt(std::cout, "----------------------------------------");
    PrintLnFmt(std::cout, "               Construction             ");
    PrintLnFmt(std::cout, "----------------------------------------");
    PrintLnFmt(std::cout, " Build time:               {:.5} ms", construction_stats.build_time);
    PrintLnFmt(std::cout, " Init time:                {:.5} ms", construction_stats.init_time / 1e6);
    PrintLnFmt(std::cout, " Memory transfer time      {:.5} ms", construction_stats.memcopy_time / 1e6);
    PrintLnFmt(std::cout, " Morton construction time: {:.5} ms", construction_stats.morton_construction_time / 1e6);
    PrintLnFmt(std::cout, " Morton sort time:         {:.5} ms", construction_stats.morton_sort_time / 1e6);
    PrintLnFmt(std::cout, " NN search time:           {:.5} ms", construction_stats.nn_search_time / 1e6);
    PrintLnFmt(std::cout, " Match and classify time:  {:.5} ms", construction_stats.match_and_classify_time / 1e6);
    PrintLnFmt(std::cout, " Prefix scan time:         {:.5} ms", construction_stats.prefix_scan_time / 1e6);
    PrintLnFmt(std::cout, " Merge and compact time:   {:.5} ms", construction_stats.merge_and_compact_time / 1e6);
    PrintLnFmt(std::cout, " SOBB refit time:          {:.5} ms", construction_stats.sobb_refit_time / 1e6);
    PrintLnFmt(std::cout, " Kernel time:              {:.5} ms", construction_stats.kernel_time / 1e6);
    PrintLnFmt(std::cout, " Node count:               {}", construction_stats.node_count);
    PrintLnFmt(std::cout, " Inner node count:         {}", construction_stats.inner_node_count);
    PrintLnFmt(std::cout, " Leaf node count:          {}", construction_stats.leaf_node_count);
    PrintLnFmt(std::cout, " BVH cost:                 {:.5}", construction_stats.bvh_cost);
    PrintLnFmt(std::cout, " Memory consumption:       {:.5} KB", construction_stats.memory_consumption / 1024.0f);
}

void ConsoleStatsExporter::ExportRaytracingStats(const RaytracingStats& rt_stats) {
    float avg_intersections_per_query = rt_stats.query_count == 0 || rt_stats.intersection_count == 0
                                            ? 0.0f
                                            : static_cast<float>(rt_stats.intersection_count) / rt_stats.query_count;
    float avg_traversals_per_query = rt_stats.query_count == 0 || rt_stats.traversal_count == 0
                                         ? 0.0f
                                         : static_cast<float>(rt_stats.traversal_count) / rt_stats.query_count;
    int64_t ray_count = rt_stats.primary_ray_count + rt_stats.secondary_ray_count + rt_stats.shadow_ray_count;
    float rays_per_second = ray_count / rt_stats.raytracing_time;

    PrintLnFmt(std::cout, "----------------------------------------");
    PrintLnFmt(std::cout, "               Ray tracing              ");
    PrintLnFmt(std::cout, "----------------------------------------");
    PrintLnFmt(std::cout, "# Ray tracing:                          ");
    PrintLnFmt(std::cout, " Frame time:               {:.5} s", rt_stats.frame_time);
    PrintLnFmt(std::cout, " Ray tracing time:         {:.5} s", rt_stats.raytracing_time);
    PrintLnFmt(std::cout, " Primary ray count:        {}", rt_stats.primary_ray_count);
    PrintLnFmt(std::cout, " Secondary ray count:      {}", rt_stats.secondary_ray_count);
    PrintLnFmt(std::cout, " Shadow ray count:         {}", rt_stats.shadow_ray_count);
    PrintLnFmt(std::cout, " Trace performance:        {:.5} Mray/s", rays_per_second / 1e6f);
    PrintLnFmt(std::cout, "# Traversal:                            ");
    PrintLnFmt(std::cout, " Query count:              {}", rt_stats.query_count);
    PrintLnFmt(std::cout, " Intersection count:       {}", rt_stats.intersection_count);
    PrintLnFmt(std::cout, " Intersections per query   {:.5}", avg_intersections_per_query);
    PrintLnFmt(std::cout, " Traversal count:          {}", rt_stats.traversal_count);
    PrintLnFmt(std::cout, " Traversals per query:     {:.5}", avg_traversals_per_query);
}

}  // namespace diplodocus

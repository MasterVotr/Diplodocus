#include "io/stats/json_stats_exporter.h"

#include <fstream>
#include <string_view>

// Third-party libraries
#include <json.hpp>

#include "util/logger.h"

namespace diplodocus {

namespace {

std::string_view ToString(RendererType renderer_type) {
    switch (renderer_type) {
        case RendererType::kCpu:
            return "cpu";
        case RendererType::kGpu:
            return "gpu";
        default:
            return "unknown";
    }
}

std::string_view ToString(AccelerationStructureType acceleration_structure_type) {
    switch (acceleration_structure_type) {
        case AccelerationStructureType::kDummy:
            return "dummy";
        case AccelerationStructureType::kSbvh:
            return "sbvh";
        case AccelerationStructureType::kPloc:
            return "ploc";
        case AccelerationStructureType::kPlocEmcVar1:
            return "ploc_emc1";
        case AccelerationStructureType::kPlocEmcVar2:
            return "ploc_emc2";
        case AccelerationStructureType::kPlocSobb:
            return "ploc_sobb";
        case AccelerationStructureType::kPlocEmcVar1Sobb:
            return "ploc_emc1_sobb";
        case AccelerationStructureType::kPlocEmcVar2Sobb:
            return "ploc_emc2_sobb";
        default:
            return "unknown";
    }
}

nlohmann::json ToJson(const Vec3& v) { return nlohmann::json::array({v.x, v.y, v.z}); }

nlohmann::json ToJson(const RenderConfig& render_config) {
    return {
        {"renderer_type", ToString(render_config.renderer_type)},
        {"width", render_config.width},
        {"height", render_config.height},
        {"background_color", ToJson(render_config.background_color)},
        {"backface_culling", render_config.backface_culling},
        {"max_depth", render_config.max_depth},
        {"area_light_sample_cnt", render_config.area_light_sample_cnt},
        {"pixel_sample_cnt", render_config.pixel_sample_cnt},
        {"seed", render_config.seed},
        {"gamma_correction", render_config.gamma_correction},
    };
}

nlohmann::json ToJson(const AccelerationStructureConfig& acceleration_structure_config) {
    return {
        {"acceleration_structure_type", ToString(acceleration_structure_config.acceleration_structure_type)},
        {"max_depth", acceleration_structure_config.max_depth},
        {"nn_search_radius", acceleration_structure_config.nn_search_radius},
        {"kdop_size", acceleration_structure_config.kdop_size},
        {"max_triangles_per_leaf", acceleration_structure_config.max_triangles_per_leaf},
        {"bin_count", acceleration_structure_config.bin_count},
    };
}

nlohmann::json ToJson(const ConstructionStats& construction_stats) {
    return {
        {"build_time", construction_stats.build_time},
        {"init_time", construction_stats.init_time / 1e6},
        {"memcopy_time", construction_stats.memcopy_time / 1e6},
        {"morton_construction_time", construction_stats.morton_construction_time / 1e6},
        {"morton_sort_time", construction_stats.morton_sort_time / 1e6},
        {"nn_search_time", construction_stats.nn_search_time / 1e6},
        {"match_and_classify_time", construction_stats.match_and_classify_time / 1e6},
        {"prefix_scan_time", construction_stats.prefix_scan_time / 1e6},
        {"merge_and_compact_time", construction_stats.merge_and_compact_time / 1e6},
        {"sobb_refit_time", construction_stats.sobb_refit_time / 1e6},
        {"kernel_time", construction_stats.kernel_time / 1e6},
        {"node_count", construction_stats.node_count},
        {"inner_node_count", construction_stats.inner_node_count},
        {"leaf_node_count", construction_stats.leaf_node_count},
        {"bvh_cost", construction_stats.bvh_cost},
        {"memory_consumption", construction_stats.memory_consumption / 1024.0f},
    };
}

nlohmann::json ToJson(const RaytracingStats& rt_stats) {
    return {
        {"frame_time", rt_stats.frame_time},
        {"raytracing_time", rt_stats.raytracing_time},
        {"primary_ray_count", rt_stats.primary_ray_count},
        {"secondary_ray_count", rt_stats.secondary_ray_count},
        {"shadow_ray_count", rt_stats.shadow_ray_count},
        {"query_count", rt_stats.query_count},
        {"intersection_count", rt_stats.intersection_count},
        {"traversal_count", rt_stats.traversal_count},
    };
}

}  // namespace

void JsonStatsExporter::ExportOne(const Config& config, const Stats& stats) {
    std::ofstream ofs(config.stats_export_config.filepath, std::ios::out | std::ios::trunc);
    if (!ofs) {
        Logger::error("JsonStatsExporter: Error opening file {}", config.stats_export_config.filepath.native());
        return;
    }

    const nlohmann::json json_output = {
        {"config",
         {
             {"render", ToJson(config.render_config)},
             {"acceleration_structure", ToJson(config.acceleration_structure_config)},
         }},
        {"stats",
         {
             {"construction", ToJson(stats.construction_stats)},
             {"raytracing", ToJson(stats.rt_stats)},
         }},
    };

    ofs << json_output.dump(2) << '\n';
    Logger::info("Exported stats to {}", config.stats_export_config.filepath.native());
}

}  // namespace diplodocus

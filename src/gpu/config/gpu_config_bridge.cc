#include "gpu/config/gpu_config_bridge.h"

#include "config/acceleration_structure_config.h"
#include "gpu/config/gpu_acceleration_structure_config.h"
#include "gpu/config/gpu_render_config.h"

namespace diplodocus::cuda_kernels {

GpuRenderConfig BridgeRenderConfig(const RenderConfig& render_config) {
    GpuRenderConfig gpu_render_config;

    gpu_render_config.background_color = {render_config.background_color.x, render_config.background_color.y,
                                          render_config.background_color.z};
    gpu_render_config.max_depth = render_config.max_depth;
    gpu_render_config.backface_culling = render_config.backface_culling;
    gpu_render_config.area_light_sample_cnt = render_config.area_light_sample_cnt;
    gpu_render_config.pixel_sample_cnt = render_config.pixel_sample_cnt;
    gpu_render_config.seed = render_config.seed;

    return gpu_render_config;
}

GpuAccelerationStructureConfig BridgeAccelerationConfig(const AccelerationStructureConfig& acceleration_config) {
    GpuAccelerationStructureConfig gpu_acceleration_config;

    switch (acceleration_config.acceleration_structure_type) {
        case AccelerationStructureType::kDummy:
            return gpu_acceleration_config;  // For Dummy no acceleration structure the config does not matter
        case AccelerationStructureType::kPloc: {
            gpu_acceleration_config.morton_type = MortonType::kMorton30;
            gpu_acceleration_config.bounding_volmue_type = BoundingVolumeType::kAabb;
            break;
        }
        case AccelerationStructureType::kPlocEmc: {
            gpu_acceleration_config.morton_type = MortonType::kEmc60;
            gpu_acceleration_config.bounding_volmue_type = BoundingVolumeType::kAabb;
            break;
        }
        case AccelerationStructureType::kPlocSobb: {
            gpu_acceleration_config.morton_type = MortonType::kMorton30;
            gpu_acceleration_config.bounding_volmue_type = BoundingVolumeType::kSobb;
            break;
        }
        case AccelerationStructureType::kPlocEmcSobb: {
            gpu_acceleration_config.morton_type = MortonType::kEmc60;
            gpu_acceleration_config.bounding_volmue_type = BoundingVolumeType::kSobb;
            break;
        }
    };

    gpu_acceleration_config.max_triangles_in_leaf = acceleration_config.max_triangles_in_leaf;
    gpu_acceleration_config.nn_search_radius = acceleration_config.nn_search_radius;
    gpu_acceleration_config.kdop_size = acceleration_config.kdop_size;

    return gpu_acceleration_config;
}

}  // namespace diplodocus::cuda_kernels

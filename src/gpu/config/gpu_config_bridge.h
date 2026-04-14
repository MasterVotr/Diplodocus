#pragma once

#include "config/acceleration_structure_config.h"
#include "config/render_config.h"
#include "gpu/config/gpu_acceleration_structure_config.h"
#include "gpu/config/gpu_render_config.h"

namespace diplodocus::cuda_kernels {

GpuRenderConfig BridgeRenderConfig(const RenderConfig& render_config);

GpuAccelerationStructureConfig BridgeAccelerationConfig(const AccelerationStructureConfig& acceleration_config);

}  // namespace diplodocus::cuda_kernels

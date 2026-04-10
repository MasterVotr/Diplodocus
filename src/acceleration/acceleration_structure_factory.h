#pragma once

#include "acceleration/acceleration_structure.h"
#include "config/acceleration_structure_config.h"

namespace diplodocus {

std::unique_ptr<AccelerationStructure> CreateAccelerationStucture(
    const AccelerationStructureConfig& acceleration_config);

}  // namespace diplodocus

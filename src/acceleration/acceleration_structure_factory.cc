#include <memory>

#include "acceleration/dummy_acceleration_structure.h"
#include "acceleration/lbvh.h"
#include "config/acceleration_structure_config.h"

namespace diplodocus {

std::unique_ptr<AccelerationStructure> CreateAccelerationStucture(
    const AccelerationStructureConfig& acceleration_config) {
    switch (acceleration_config.acceleration_structure_type) {
        case AccelerationStructureType::kDummy:
            return std::make_unique<DummyAccelerationStucture>();
        case AccelerationStructureType::kLbvh:
            return std::make_unique<LBvh>();
        default:
            throw std::invalid_argument("Unkown AccelerationStructureType");
    }
}

}  // namespace diplodocus

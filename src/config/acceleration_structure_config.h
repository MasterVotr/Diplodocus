#pragma once

#include <stdexcept>

#include "util/util.h"

namespace diplodocus {

enum class AccelerationStructureType { kDummy, kLbvh };

template <>
inline AccelerationStructureType ParseEnum<AccelerationStructureType>(std::string_view sv) {
    if (sv == "dummy") return AccelerationStructureType::kDummy;
    if (sv == "lbvh") return AccelerationStructureType::kLbvh;
    throw std::runtime_error("Invalid AccelerationStructureType: " + std::string(sv));
}

struct AccelerationStructureConfig {
    AccelerationStructureType acceleration_structure_type = AccelerationStructureType::kDummy;
    int max_depth = 16;
    int max_triangles_in_leaf = 4;
};

}  // namespace diplodocus

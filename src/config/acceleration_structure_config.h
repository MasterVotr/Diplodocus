#pragma once

#include <stdexcept>

#include "util/util.h"

namespace diplodocus {

enum class AccelerationStructureType { kDummy, kSbvh, kPloc, kPlocEmc, kPlocSobb, kPlocEmcSobb };

template <>
inline AccelerationStructureType ParseEnum<AccelerationStructureType>(std::string_view sv) {
    if (sv == "dummy") return AccelerationStructureType::kDummy;
    if (sv == "sbvh") return AccelerationStructureType::kSbvh;
    if (sv == "ploc") return AccelerationStructureType::kPloc;
    if (sv == "ploc_emc") return AccelerationStructureType::kPlocEmc;
    if (sv == "ploc_sobb") return AccelerationStructureType::kPlocSobb;
    if (sv == "ploc_emc_sobb") return AccelerationStructureType::kPlocEmcSobb;
    throw std::runtime_error("Invalid AccelerationStructureType: " + std::string(sv));
}

struct AccelerationStructureConfig {
    AccelerationStructureType acceleration_structure_type = AccelerationStructureType::kDummy;
    int max_depth = 32;
    int nn_search_radius = 10;
    int kdop_size = 32;
    int max_triangles_per_leaf = 4;
    int bin_count = 16;
};

}  // namespace diplodocus

#pragma once

#include <stdexcept>

#include "util/util.h"

namespace diplodocus {

enum class AccelerationStructureType {
    kDummy,
    kSbvh,
    kPloc,
    kPlocEmcVar1,
    kPlocEmcVar2,
    kPlocSobb,
    kPlocEmcVar1Sobb,
    kPlocEmcVar2Sobb
};

template <>
inline AccelerationStructureType ParseEnum<AccelerationStructureType>(std::string_view sv) {
    if (sv == "dummy") return AccelerationStructureType::kDummy;
    if (sv == "sbvh") return AccelerationStructureType::kSbvh;
    if (sv == "ploc") return AccelerationStructureType::kPloc;
    if (sv == "ploc_emc1") return AccelerationStructureType::kPlocEmcVar1;
    if (sv == "ploc_emc2") return AccelerationStructureType::kPlocEmcVar2;
    if (sv == "ploc_sobb") return AccelerationStructureType::kPlocSobb;
    if (sv == "ploc_emc1_sobb") return AccelerationStructureType::kPlocEmcVar1Sobb;
    if (sv == "ploc_emc2_sobb") return AccelerationStructureType::kPlocEmcVar2Sobb;
    throw std::runtime_error("Invalid AccelerationStructureType: " + std::string(sv));
}

struct AccelerationStructureConfig {
    AccelerationStructureType acceleration_structure_type = AccelerationStructureType::kPloc;
    int max_depth = 32;
    int nn_search_radius = 25;
    int kdop_size = 32;
    int max_triangles_per_leaf = 4;
    int bin_count = 16;
};

}  // namespace diplodocus

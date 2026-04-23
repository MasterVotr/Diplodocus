#pragma once

#include <cuda_runtime_api.h>
#include <driver_types.h>

#include <cstdint>

#include "gpu/acceleration/gpu_aabb.h"
#include "gpu/config/gpu_acceleration_structure_config.h"
#include "gpu/cuda_compat.h"
#include "gpu/cuda_math.h"

namespace diplodocus::cuda_kernels {

template <MortonType M>
class MortonTrait;

template <>
class MortonTrait<MortonType::kMorton32> {
   public:
    using CodeT = uint32_t;

    struct Setup {
        float3 scene_min;
        float3 scene_size;
    };

    static Setup Init(const GpuAabb& scene_aabb) { return {scene_aabb.min, scene_aabb.max - scene_aabb.min}; }

    static DI CodeT Encode(float3 centroid, Setup setup) {
        float3 nc;
        nc.x = setup.scene_size.x > kEpsilon ? (centroid.x - setup.scene_min.x) / setup.scene_size.x : 0.5f;
        nc.y = setup.scene_size.y > kEpsilon ? (centroid.y - setup.scene_min.y) / setup.scene_size.y : 0.5f;
        nc.z = setup.scene_size.z > kEpsilon ? (centroid.z - setup.scene_min.z) / setup.scene_size.z : 0.5f;
        float x = Clamp(nc.x * 1023, 0.0f, 1023.0f);
        float y = Clamp(nc.y * 1023, 0.0f, 1023.0f);
        float z = Clamp(nc.z * 1023, 0.0f, 1023.0f);
        uint32_t xx = ExpandBits(static_cast<CodeT>(x));
        uint32_t yy = ExpandBits(static_cast<CodeT>(y));
        uint32_t zz = ExpandBits(static_cast<CodeT>(z));
        return xx << 2 | yy << 1 | zz;
    }

   private:
    static DI CodeT ExpandBits(CodeT x) {
        x &= 0x3ff;  // trim to 10 bits
        x = (x | x << 16) & 0x30000ff;
        x = (x | x << 8) & 0x300f00f;
        x = (x | x << 4) & 0x30c30c3;
        x = (x | x << 2) & 0x9249249;
        return x;
    }
};

// DI uint64_t ExpandBits(uint64_t x) {
//     x &= 0x1fffff;
//     x = (x | x << 32) & 0x1f00000000ffff;
//     x = (x | x << 16) & 0x1f0000ff0000ff;
//     x = (x | x << 8) & 0x100f00f00f00f00f;
//     x = (x | x << 4) & 0x10c30c30c30c30c3;
//     x = (x | x << 2) & 0x1249249249249249;
//     return x;
// }
//
// DI uint64_t Morton63(float3 p) {
//     float x = Clamp(p.x * 2'097'151, 0.0f, 2'097'151.0f);
//     float y = Clamp(p.y * 2'097'151, 0.0f, 2'097'151.0f);
//     float z = Clamp(p.z * 2'097'151, 0.0f, 2'097'151.0f);
//     uint64_t xx = ExpandBits(static_cast<uint64_t>(x));
//     uint64_t yy = ExpandBits(static_cast<uint64_t>(y));
//     uint64_t zz = ExpandBits(static_cast<uint64_t>(z));
//     return xx << 2 | yy << 1 | zz;
// }

}  // namespace diplodocus::cuda_kernels

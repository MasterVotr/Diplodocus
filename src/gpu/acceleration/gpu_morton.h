#pragma once

#include <cuda_runtime_api.h>
#include <driver_types.h>
#include <vector_types.h>

#include <algorithm>
#include <cstdint>

#include "gpu/acceleration/gpu_aabb.h"
#include "gpu/config/gpu_acceleration_structure_config.h"
#include "gpu/cuda_compat.h"
#include "gpu/cuda_math.h"

namespace diplodocus::cuda_kernels {

template <MortonType M>
class MortonTrait;

// template <>
// class MortonTrait<MortonType::kMorton32> {
//    public:
//     using CodeT = uint32_t;
//
//     struct Setup {
//         float3 scene_min;
//         float3 scene_size;
//     };
//
//     static Setup Init(const GpuAabb& scene_aabb) { return {scene_aabb.min, scene_aabb.max - scene_aabb.min}; }
//
//     static DI CodeT Encode(const GpuAabb& aabb, const Setup& setup) {
//         float3 centroid = (aabb.min + aabb.max) * 0.5f;
//         float3 v = centroid - setup.scene_min;
//         float3 nc;
//         nc.x = setup.scene_size.x > kEpsilon ? (v.x - setup.scene_min.x) / setup.scene_size.x : 0.5f;
//         nc.y = setup.scene_size.y > kEpsilon ? (v.y - setup.scene_min.y) / setup.scene_size.y : 0.5f;
//         nc.z = setup.scene_size.z > kEpsilon ? (v.z - setup.scene_min.z) / setup.scene_size.z : 0.5f;
//         float x = Clamp(nc.x * 1023, 0.0f, 1023.0f);
//         float y = Clamp(nc.y * 1023, 0.0f, 1023.0f);
//         float z = Clamp(nc.z * 1023, 0.0f, 1023.0f);
//         CodeT xx = ExpandBits(static_cast<CodeT>(x));
//         CodeT yy = ExpandBits(static_cast<CodeT>(y));
//         CodeT zz = ExpandBits(static_cast<CodeT>(z));
//         return xx << 2 | yy << 1 | zz;
//     }
//
//    private:
//     static DI CodeT ExpandBits(CodeT x) {
//         x &= 0x3ff;  // trim to 10 bits
//         x = (x | x << 16) & 0x30000ff;
//         x = (x | x << 8) & 0x300f00f;
//         x = (x | x << 4) & 0x30c30c3;
//         x = (x | x << 2) & 0x9249249;
//         return x;
//     }
// };

template <>
class MortonTrait<MortonType::kMorton64> {
   public:
    using CodeT = uint64_t;

    struct Setup {
        float3 scene_min;
        float3 scene_size;
    };

    static Setup Init(const GpuAabb& scene_aabb) { return {scene_aabb.min, scene_aabb.max - scene_aabb.min}; }

    static DI CodeT Encode(const GpuAabb& aabb, const Setup& setup) {
        float3 centroid = (aabb.min + aabb.max) * 0.5f;
        float3 nc;
        nc.x = setup.scene_size.x > kEpsilon ? (centroid.x - setup.scene_min.x) / setup.scene_size.x : 0.5f;
        nc.y = setup.scene_size.y > kEpsilon ? (centroid.y - setup.scene_min.y) / setup.scene_size.y : 0.5f;
        nc.z = setup.scene_size.z > kEpsilon ? (centroid.z - setup.scene_min.z) / setup.scene_size.z : 0.5f;
        float x = Clamp(nc.x * 2'097'151, 0.0f, 2'097'151.0f);
        float y = Clamp(nc.y * 2'097'151, 0.0f, 2'097'151.0f);
        float z = Clamp(nc.z * 2'097'151, 0.0f, 2'097'151.0f);
        CodeT xx = ExpandBits(static_cast<CodeT>(x));
        CodeT yy = ExpandBits(static_cast<CodeT>(y));
        CodeT zz = ExpandBits(static_cast<CodeT>(z));
        return xx << 2 | yy << 1 | zz;
    }

   private:
    static DI CodeT ExpandBits(CodeT x) {
        x &= 0x1fffff;  // trim to 21 bits
        x = (x | x << 32) & 0x1f00000000ffff;
        x = (x | x << 16) & 0x1f0000ff0000ff;
        x = (x | x << 8) & 0x100f00f00f00f00f;
        x = (x | x << 4) & 0x10c30c30c30c30c3;
        x = (x | x << 2) & 0x1249249249249249;
        return x;
    }
};

template <>
class MortonTrait<MortonType::kEmc64Var1> {
   public:
    using CodeT = uint64_t;
    constexpr static int kBitsPerCode{sizeof(CodeT) * 8};
    constexpr static int kBitsPerAxis{kBitsPerCode / 4};
    constexpr static int kMaxValue{(CodeT(1) << kBitsPerAxis) - 1};

    struct Setup {
        float3 scene_size;
        float3 scene_min;
        int axis_order[3]{0, 1, 2};  // Axis order (based on scene dimension)
        float axis_scales[4]{0};     // Quantization scales
    };

    static Setup Init(const GpuAabb& scene_aabb) {
        float3 scene_size = scene_aabb.max - scene_aabb.min;
        float scene_diagonal_length = Length(scene_size);
        float scene_size_arr[3]{scene_size.x, scene_size.y, scene_size.z};

        Setup setup{};
        setup.scene_min = scene_aabb.min;
        setup.scene_size = scene_size;

        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                if (scene_size_arr[setup.axis_order[j]] < scene_size_arr[setup.axis_order[j + 1]]) {
                    std::swap(setup.axis_order[j], setup.axis_order[j + 1]);
                }
            }
        }

        // Quantization scales
        setup.axis_scales[0] =
            scene_size_arr[setup.axis_order[0]] > kEpsilon ? kMaxValue / scene_size_arr[setup.axis_order[0]] : 1.0f;
        setup.axis_scales[1] =
            scene_size_arr[setup.axis_order[1]] > kEpsilon ? kMaxValue / scene_size_arr[setup.axis_order[1]] : 1.0f;
        setup.axis_scales[2] =
            scene_size_arr[setup.axis_order[2]] > kEpsilon ? kMaxValue / scene_size_arr[setup.axis_order[2]] : 1.0f;
        setup.axis_scales[3] = scene_diagonal_length > kEpsilon ? kMaxValue / scene_diagonal_length : 1.0f;

        return setup;
    }

    static DI CodeT Encode(const GpuAabb& aabb, const Setup& setup) {
        float3 centroid = (aabb.min + aabb.max) * 0.5f;
        float3 v = centroid - setup.scene_min;
        float diagonal_length = Length(aabb.max - aabb.min);
        float v_arr[3]{v.x, v.y, v.z};
        CodeT v0 = static_cast<CodeT>(
            Clamp(setup.axis_scales[0] * v_arr[setup.axis_order[0]], 0.0f, static_cast<float>(kMaxValue)));
        CodeT v1 = static_cast<CodeT>(
            Clamp(setup.axis_scales[1] * v_arr[setup.axis_order[1]], 0.0f, static_cast<float>(kMaxValue)));
        CodeT v2 = static_cast<CodeT>(
            Clamp(setup.axis_scales[2] * v_arr[setup.axis_order[2]], 0.0f, static_cast<float>(kMaxValue)));
        CodeT v3 =
            static_cast<CodeT>(Clamp(setup.axis_scales[3] * diagonal_length, 0.0f, static_cast<float>(kMaxValue)));
        return ExpandBits(v0, setup) << 3 | ExpandBits(v1, setup) << 2 | ExpandBits(v2, setup) << 1 |
               ExpandBits(v3, setup);
    }

   private:
    static DI CodeT ExpandBits(CodeT x, const Setup&) {
        CodeT v = 0;
        CodeT mask = 1;
        for (int i = 0; i < kBitsPerAxis; i++) {
            v |= ((x & mask) << (3 * i));
            mask <<= 1;
        }
        return v;
    }
};

template <>
class MortonTrait<MortonType::kEmc64Var2> {
   public:
    using CodeT = uint64_t;
    constexpr static int kBitsPerCode{sizeof(CodeT) * 8};
    constexpr static int kSizeBitFrequency{7};
    constexpr static int kMaxSizeBits{6};

    struct Setup {
        float3 scene_size;
        float3 scene_min;
        int axes_layout[kBitsPerCode]{0};     // Bit order
        int bits_per_axis[4]{0};              // Bit count per axis
        int bit_shifts[4 * kBitsPerCode]{0};  // Bit shift in final code
        float axis_scales[4]{0};              // Quantization scales
    };

    static Setup Init(const GpuAabb& scene_aabb) {
        float3 scene_size = scene_aabb.max - scene_aabb.min;
        float scene_diagonal_length = Length(scene_size);

        Setup setup{};
        setup.scene_min = scene_aabb.min;
        setup.scene_size = scene_size;

        for (int i = 0; i < kBitsPerCode; i++) {
            int axis;
            // Insert size bit
            if (i % kSizeBitFrequency == kSizeBitFrequency - 1 && setup.bits_per_axis[3] < kMaxSizeBits) {
                axis = 3;
            }
            // Pick the largest axis and halve it
            else {
                if (scene_size.x > scene_size.y) {
                    if (scene_size.x > scene_size.z) {
                        axis = 0;
                        scene_size.x /= 2.0f;
                    } else {
                        axis = 2;
                        scene_size.z /= 2.0f;
                    }
                } else {
                    if (scene_size.y > scene_size.z) {
                        axis = 1;
                        scene_size.y /= 2.0f;
                    } else {
                        axis = 2;
                        scene_size.z /= 2.0f;
                    }
                }
            }
            setup.axes_layout[i] = axis;
            setup.bit_shifts[axis * kBitsPerCode + setup.bits_per_axis[axis]] = i;
            setup.bits_per_axis[axis]++;
        }

        // Quantization scales
        setup.axis_scales[0] = Pow(2, setup.bits_per_axis[0]) / setup.scene_size.x;
        setup.axis_scales[1] = Pow(2, setup.bits_per_axis[1]) / setup.scene_size.y;
        setup.axis_scales[2] = Pow(2, setup.bits_per_axis[2]) / setup.scene_size.z;
        setup.axis_scales[3] = Pow(2, setup.bits_per_axis[3]) / scene_diagonal_length;

        return setup;
    }

    static DI CodeT Encode(const GpuAabb& aabb, const Setup& setup) {
        float3 centroid = (aabb.min + aabb.max) * 0.5f;
        float3 v = centroid - setup.scene_min;
        float diagonal_length = Length(aabb.max - aabb.min);
        CodeT v0 = static_cast<CodeT>(
            Clamp(setup.axis_scales[0] * v.x, 0.0f, static_cast<float>(CodeT(1) << setup.bits_per_axis[0])));
        CodeT v1 = static_cast<CodeT>(
            Clamp(setup.axis_scales[1] * v.y, 0.0f, static_cast<float>(CodeT(1) << setup.bits_per_axis[1])));
        CodeT v2 = static_cast<CodeT>(
            Clamp(setup.axis_scales[2] * v.z, 0.0f, static_cast<float>(CodeT(1) << setup.bits_per_axis[2])));
        CodeT v3 = static_cast<CodeT>(Clamp(setup.axis_scales[3] * diagonal_length, 0.0f,
                                            static_cast<float>(CodeT(1) << setup.bits_per_axis[3])));
        return (ExpandBits(0, v0, setup) | ExpandBits(1, v1, setup) | ExpandBits(2, v2, setup) |
                ExpandBits(3, v3, setup));
    }

   private:
    static DI CodeT ExpandBits(int a, CodeT x, const Setup& setup) {
        CodeT v = 0;
        CodeT mask = 1;
        // CodeT mask = 1ULL << (setup.bits_per_axis[a] - 1);
        for (int i = 0; i < setup.bits_per_axis[a]; i++) {
            v |= ((x & mask) << (setup.bit_shifts[a * kBitsPerCode + i] - i));
            mask <<= 1;
            // mask >>= 1;
        }
        return v;
    }
};

}  // namespace diplodocus::cuda_kernels

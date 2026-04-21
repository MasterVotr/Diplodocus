#pragma once

#include <cstdint>

#include "gpu/cuda_compat.h"
#include "gpu/cuda_math.h"

namespace diplodocus::cuda_kernels {

namespace {

DI uint32_t ExpandBits(uint32_t x) {
    x &= 0x3ff;  // trim to 10 bits
    x = (x | x << 16) & 0x30000ff;
    x = (x | x << 8) & 0x300f00f;
    x = (x | x << 4) & 0x30c30c3;
    x = (x | x << 2) & 0x9249249;
    return x;
}

DI uint64_t ExpandBits(uint64_t x) {
    x &= 0x1fffff;
    x = (x | x << 32) & 0x1f00000000ffff;
    x = (x | x << 16) & 0x1f0000ff0000ff;
    x = (x | x << 8) & 0x100f00f00f00f00f;
    x = (x | x << 4) & 0x10c30c30c30c30c3;
    x = (x | x << 2) & 0x1249249249249249;
    return x;
}

}  // namespace

DI uint32_t Morton30(float3 p) {
    float x = Clamp(p.x * 1023, 0.0f, 1023.0f);
    float y = Clamp(p.y * 1023, 0.0f, 1023.0f);
    float z = Clamp(p.z * 1023, 0.0f, 1023.0f);
    uint32_t xx = ExpandBits(static_cast<uint32_t>(x));
    uint32_t yy = ExpandBits(static_cast<uint32_t>(y));
    uint32_t zz = ExpandBits(static_cast<uint32_t>(z));
    return xx << 2 | yy << 1 | zz;
}

DI uint64_t Morton63(float3 p) {
    float x = Clamp(p.x * 2'097'151, 0.0f, 2'097'151.0f);
    float y = Clamp(p.y * 2'097'151, 0.0f, 2'097'151.0f);
    float z = Clamp(p.z * 2'097'151, 0.0f, 2'097'151.0f);
    uint64_t xx = ExpandBits(static_cast<uint64_t>(x));
    uint64_t yy = ExpandBits(static_cast<uint64_t>(y));
    uint64_t zz = ExpandBits(static_cast<uint64_t>(z));
    return xx << 2 | yy << 1 | zz;
}

}  // namespace diplodocus::cuda_kernels

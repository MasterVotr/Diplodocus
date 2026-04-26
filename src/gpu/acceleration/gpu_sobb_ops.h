#pragma once

#include <vector_types.h>

#include <cassert>
#include <cmath>
#include <cstdint>

#include "gpu/acceleration/gpu_aabb.h"
#include "gpu/acceleration/gpu_sobb.h"
#include "gpu/cuda_compat.h"
#include "gpu/cuda_math.h"
#include "gpu/scene/gpu_ray.h"

namespace diplodocus::cuda_kernels {

namespace {

constexpr float kIntersectionEpsilon = 1e-5f;

// From SOBB article implementation
static C float3 k32dop[16] = {
    // axes
    {1.0000000, 0.0000000, 0.0000000},   {0.0000000, 1.0000000, 0.0000000},  {0.0000000, 0.0000000, 1.0000000},
    {0.5773500, 0.5773500, 0.5773500},   {0.5773500, 0.5773500, -0.5773500}, {0.5773500, -0.5773500, 0.5773500},
    {0.5773500, -0.5773500, -0.5773500}, {0.8832792, 0.2360192, 0.4051083},  {0.5164694, -0.1144751, -0.8486195},
    {0.8587868, 0.1252680, -0.4967829},  {0.5534041, -0.0057654, 0.8328929}, {0.1407341, -0.8645653, -0.4824112},
    {0.5901749, -0.8072627, -0.0045526}, {0.7372449, 0.6756153, 0.0037376},  {0.8835392, -0.2404428, 0.4019275},
    {0.0025974, -0.5183646, -0.8551558},
};

H D I uint32_t pack_normal_idx(uint8_t i0, uint8_t i1, uint8_t i2) { return (i0 << 20) | (i1 << 10) | (i2); }
H D I uint8_t unpack_normal_idx(uint32_t n_id, int idx) { return (n_id >> (20 - (idx * 10))) & 0x3FF; }

}  // namespace

D I bool IntersectRaySobb(const GpuRay& ray, const GpuSobb& sobb, float& t_min, float& t_max) {
    const float3& n0 = k32dop[unpack_normal_idx(sobb.n_ids, 0)];
    const float3& n1 = k32dop[unpack_normal_idx(sobb.n_ids, 1)];
    const float3& n2 = k32dop[unpack_normal_idx(sobb.n_ids, 2)];

    const float3 n_dir{DotSafe(n0, ray.dir, kIntersectionEpsilon), DotSafe(n1, ray.dir, kIntersectionEpsilon),
                       DotSafe(n2, ray.dir, kIntersectionEpsilon)};
    const float3 n_origin{Dot(n0, ray.origin), Dot(n1, ray.origin), Dot(n2, ray.origin)};

    const float3 n_inv_d = Splat(1.0f) / n_dir;
    const float3 t0 = (sobb.b_mins - n_origin) * n_inv_d;
    const float3 t1 = (sobb.b_maxs - n_origin) * n_inv_d;
    const float3 t_smaller = Fmin(t0, t1);
    const float3 t_bigger = Fmax(t0, t1);

    t_min = Fmax(Fmax(t_smaller.x, t_smaller.y), t_smaller.z);
    t_max = Fmin(Fmin(t_bigger.x, t_bigger.y), t_bigger.z);

    return t_max >= t_min;
}

H D I float CalculateSobbSurfaceArea(const GpuSobb& sobb) {
    const float3& n0 = k32dop[unpack_normal_idx(sobb.n_ids, 0)];
    const float3& n1 = k32dop[unpack_normal_idx(sobb.n_ids, 1)];
    const float3& n2 = k32dop[unpack_normal_idx(sobb.n_ids, 2)];

    float det = Determinant3(n0, n1, n2);

    const float& d0 = sobb.b_maxs.x - sobb.b_mins.x;
    const float& d1 = sobb.b_maxs.y - sobb.b_mins.y;
    const float& d2 = sobb.b_maxs.z - sobb.b_mins.z;

    float area = d0 * d1 + d1 * d2 + d2 * d0;
    area = 2.0f * Abs(area) / det;
#ifdef __CUDACC__
    return isinf(area) ? 0.0f : area;
#else
    return std::isinf(area) ? 0.0f : area;
#endif
}

D I GpuDop CreateDopFromTriangle(const float3& v0, const float3& v1, const float3& v2) {
    GpuDop dop{};

#pragma unroll
    for (int i = 0; i < 16; i++) {
        float3 n = k32dop[i];

        float d0 = Dot(n, v0);
        float d1 = Dot(n, v1);
        float d2 = Dot(n, v2);

        dop.slab[i].x = Fmin(d0, Fmin(d1, d2));
        dop.slab[i].y = Fmax(d0, Fmax(d1, d2));
    }

    return dop;
}

D I GpuSobb CreateSobbFromDopK2(const GpuDop& dop) {
    float d[16]{};
    uint8_t k0 = 0;  // First basis
    uint8_t k1 = 1;  // Second basis
    uint8_t k2 = 2;  // Third basis

    // Choose first basis (lowest extent)
#pragma unroll
    for (uint8_t i = 0; i < 16; i++) {
        d[i] = dop.slab[i].y - dop.slab[i].x;
        if (d[i] < d[k0]) {
            k0 = i;
        }
    }
    float3 n0 = k32dop[k0];

    // Debug: try to use standard basis - DID NOT WORK
    // GpuSobb debug_sobb;
    // debug_sobb.n_ids = pack_normal_idx(0, 1, 2);
    // debug_sobb.b_mins = {dop.slab[0].x, dop.slab[1].x, dop.slab[2].x};
    // debug_sobb.b_maxs = {dop.slab[0].y, dop.slab[1].y, dop.slab[2].y};
    // return debug_sobb;

    // Chose second and third basis (smallest surface area)
    float best_sa = kInfinity;
#pragma unroll
    for (uint8_t j = 0; j < 16; j++) {
        if (j == k0) continue;
        const float3 n1 = k32dop[j];
        const float detX = n0.y * n1.z - n0.z * n1.y;
        const float detY = n0.z * n1.x - n0.x * n1.z;
        const float detZ = n0.x * n1.y - n0.y * n1.x;
        const float a0 = d[k0] * d[j];
        const float a1 = d[k0] + d[j];

#pragma unroll
        for (uint8_t k = 0; k < 16; k++) {
            if (k == k0 || k == j) continue;
            const float3 n2 = k32dop[k];
            const float det = Abs(detX * n2.x + detY * n2.y + detZ * n2.z);
            {
                float sa = a0 + d[k] * a1;
                sa = 2 * Abs(sa) / det;
                if (sa < best_sa) {
                    best_sa = sa;
                    k1 = j;
                    k2 = k;
                }
            }
        }
    }
    GpuSobb sobb;
    sobb.b_mins = {dop.slab[k0].x, dop.slab[k1].x, dop.slab[k2].x};
    sobb.b_maxs = {dop.slab[k0].y, dop.slab[k1].y, dop.slab[k2].y};
    sobb.n_ids = pack_normal_idx(k0, k1, k2);

    // Debug: try to inflate the bv a bit - DID NOT WORK
    // float eps = 1e-5f;
    // sobb.b_maxs = sobb.b_maxs + Splat(eps);
    // sobb.b_mins = sobb.b_mins - Splat(eps);

    return sobb;
}

D I void FitDop2Dop(GpuDop& dop_a, const GpuDop& dop_b) {
#pragma unroll
    for (int i = 0; i < 16; i++) {
        dop_a.slab[i].x = Fmin(dop_a.slab[i].x, dop_b.slab[i].x);
        dop_a.slab[i].y = Fmax(dop_a.slab[i].y, dop_b.slab[i].y);
    }
}

D I GpuSobb CreateSobbFromAabb(const GpuAabb& aabb) {
    GpuSobb sobb;

    sobb.n_ids = pack_normal_idx(0, 1, 2);
    sobb.b_mins = aabb.min;
    sobb.b_maxs = aabb.max;

    return sobb;
}

}  // namespace diplodocus::cuda_kernels

#pragma once

#include <cuda_runtime.h>
#include <vector_functions.h>

#include <cmath>

#include "gpu/cuda_compat.h"

namespace diplodocus::cuda_kernels {

constexpr float kEpsilon = 1e-6;
constexpr float kInfinity = MAXFLOAT;

DI float Pow(float b, float e) { return pow(b, e); }
DI float Sqrt(float a) { return sqrtf(a); }
DI float Rsqrt(float a) { return rsqrtf(a); }
DI float Fmin(float a, float b) { return fminf(a, b); }
DI float Fmax(float a, float b) { return fmaxf(a, b); }
DI float Abs(float a) { return fabsf(a); }
DI float Clamp(float a, float lo, float hi) { return Fmin(Fmax(a, lo), hi); }

// constructors
DI float3 Splat(float s) { return make_float3(s, s, s); }

// ops (prefer make_float3)
DI float3 operator-(float3 a) { return make_float3(-a.x, -a.y, -a.z); }
DI float3 operator+(float3 a, float3 b) { return make_float3(a.x + b.x, a.y + b.y, a.z + b.z); }
DI float3 operator+(float3 a, float s) { return make_float3(a.x + s, a.y + s, a.z + s); }
DI float3 operator+(float s, float3 a) { return make_float3(a.x + s, a.y + s, a.z + s); }
DI float3 operator-(float3 a, float3 b) { return make_float3(a.x - b.x, a.y - b.y, a.z - b.z); }
DI float3 operator*(float3 a, float s) { return make_float3(a.x * s, a.y * s, a.z * s); }
DI float3 operator*(float s, float3 a) { return make_float3(a.x * s, a.y * s, a.z * s); }
DI float3 operator*(float3 a, float3 b) { return make_float3(a.x * b.x, a.y * b.y, a.z * b.z); }
DI float3 operator/(float3 a, float s) { return a * (1.0f / s); }
DI float3 operator/(float3 a, float3 b) { return make_float3(a.x / b.x, a.y / b.y, a.z / b.z); }

// math
DI float3 Fmin(float3 a, float3 b) { return make_float3(Fmin(a.x, b.x), Fmin(a.y, b.y), Fmin(a.z, b.z)); }
DI float3 Fmax(float3 a, float3 b) { return make_float3(Fmax(a.x, b.x), Fmax(a.y, b.y), Fmax(a.z, b.z)); }
DI float3 Abs(float3 v) { return make_float3(Abs(v.x), Abs(v.y), Abs(v.z)); }
DI float3 Clamp(float3 x, float3 lo, float3 hi) { return Fmin(Fmax(x, lo), hi); }

DI float Dot(float3 a, float3 b) { return fmaf(a.x, b.x, fmaf(a.y, b.y, a.z * b.z)); }
DI float Length(float3 v) { return Sqrt(Dot(v, v)); }
DI float3 Normalize(float3 v) { return v * Rsqrt(Dot(v, v)); }
DI float3 NormalizeSafe(float3 v, float eps = kEpsilon) {
    float len2 = Dot(v, v);
    if (len2 <= eps * eps) return make_float3(0.f, 0.f, 0.f);
    return v * Rsqrt(len2);
}
DI float3 Cross(float3 a, float3 b) {
    return make_float3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
DI float3 Lerp(float3 a, float3 b, float t) { return a + (b - a) * t; }
DI bool AlmostEqual(float3 a, float3 b, float eps = kEpsilon) {
    return Abs(a.x - b.x) <= eps && Abs(a.y - b.y) <= eps && Abs(a.z - b.z) <= eps;
}

}  // namespace diplodocus::cuda_kernels

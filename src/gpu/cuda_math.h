#pragma once

#include <cuda_runtime.h>
#include <vector_functions.h>
#include <vector_types.h>

#include <cmath>

#include "gpu/cuda_compat.h"

namespace diplodocus::cuda_kernels {

constexpr float kEpsilon = 1e-6;
constexpr float kInfinity = MAXFLOAT;

HDI float Pow(float b, float e) { return pow(b, e); }
HDI float Sqrt(float a) { return sqrtf(a); }
HDI float Fmin(float a, float b) { return fminf(a, b); }
HDI float Fmax(float a, float b) { return fmaxf(a, b); }
HDI float Abs(float a) { return fabsf(a); }
HDI float Clamp(float a, float lo, float hi) { return Fmin(Fmax(a, lo), hi); }

// constructors
HDI float3 Splat(float s) { return make_float3(s, s, s); }

// ops (prefer make_float3)
HDI float3 operator-(float3 a) { return make_float3(-a.x, -a.y, -a.z); }
HDI float3 operator+(float3 a, float3 b) { return make_float3(a.x + b.x, a.y + b.y, a.z + b.z); }
HDI float3 operator+(float3 a, float s) { return make_float3(a.x + s, a.y + s, a.z + s); }
HDI float3 operator+(float s, float3 a) { return make_float3(a.x + s, a.y + s, a.z + s); }
HDI float3 operator-(float3 a, float3 b) { return make_float3(a.x - b.x, a.y - b.y, a.z - b.z); }
HDI float3 operator*(float3 a, float s) { return make_float3(a.x * s, a.y * s, a.z * s); }
HDI float3 operator*(float s, float3 a) { return make_float3(a.x * s, a.y * s, a.z * s); }
HDI float3 operator*(float3 a, float3 b) { return make_float3(a.x * b.x, a.y * b.y, a.z * b.z); }
HDI float3 operator/(float3 a, float s) { return a * (1.0f / s); }
HDI float3 operator/(float3 a, float3 b) { return make_float3(a.x / b.x, a.y / b.y, a.z / b.z); }

// compare
HDI bool LessAny(float3 a, float3 b) { return a.x < b.x || a.y < b.y || a.z < b.z; }
HDI bool LessAll(float3 a, float3 b) { return a.x < b.x && a.y < b.y && a.z < b.z; }

// math
HDI float3 Pow(float3 b, float e) { return make_float3(Pow(b.x, e), Pow(b.y, e), Pow(b.z, e)); }
HDI float3 Fmin(float3 a, float3 b) { return make_float3(Fmin(a.x, b.x), Fmin(a.y, b.y), Fmin(a.z, b.z)); }
HDI float3 Fmax(float3 a, float3 b) { return make_float3(Fmax(a.x, b.x), Fmax(a.y, b.y), Fmax(a.z, b.z)); }
HDI float3 Abs(float3 v) { return make_float3(Abs(v.x), Abs(v.y), Abs(v.z)); }
HDI float3 Clamp(float3 x, float3 lo, float3 hi) { return Fmin(Fmax(x, lo), hi); }

HDI float Dot(float3 a, float3 b) { return fmaf(a.x, b.x, fmaf(a.y, b.y, a.z * b.z)); }
HDI float Length(float3 v) { return Sqrt(Dot(v, v)); }
HDI float3 Normalize(float3 v) { return v / Sqrt(Dot(v, v)); }
HDI float3 NormalizeSafe(float3 v, float eps = kEpsilon) {
    float len2 = Dot(v, v);
    if (len2 <= eps * eps) return make_float3(0.f, 0.f, 0.f);
    return v / Sqrt(len2);
}
HDI float3 Cross(float3 a, float3 b) {
    return make_float3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
HDI float3 Lerp(float3 a, float3 b, float t) { return a + (b - a) * t; }
HDI bool AlmostEqual(float3 a, float3 b, float eps = kEpsilon) {
    return Abs(a.x - b.x) <= eps && Abs(a.y - b.y) <= eps && Abs(a.z - b.z) <= eps;
}

}  // namespace diplodocus::cuda_kernels

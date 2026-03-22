#pragma once

#include <cassert>
#include <cmath>
#include <cstddef>
#include <ostream>
#include <type_traits>

#include "util/util.h"

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    constexpr Vec3() noexcept = default;
    constexpr explicit Vec3(float v) noexcept : x(v), y(v), z(v) {}
    constexpr Vec3(float x_, float y_, float z_ = 0.0f) noexcept : x(x_), y(y_), z(z_) {}

    constexpr float& operator[](size_t i) noexcept { return i == 0 ? x : i == 1 ? y : z; }
    constexpr const float& operator[](size_t i) const noexcept { return i == 0 ? x : i == 1 ? y : z; }

    // Unary operators
    [[nodiscard]] constexpr Vec3 operator-() const noexcept { return {-x, -y, -z}; }

    // Vector-vector operators
    constexpr Vec3& operator+=(const Vec3& v) noexcept {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }
    constexpr Vec3& operator-=(const Vec3& v) noexcept {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        return *this;
    }
    constexpr Vec3& operator*=(const Vec3& v) noexcept {
        x *= v.x;
        y *= v.y;
        z *= v.z;
        return *this;
    }
    constexpr Vec3& operator/=(const Vec3& v) noexcept {
        x /= v.x;
        y /= v.y;
        z /= v.z;
        return *this;
    }

    // Vector-scalar operators
    constexpr Vec3& operator+=(float s) noexcept {
        x += s;
        y += s;
        z += s;
        return *this;
    }
    constexpr Vec3& operator-=(float s) noexcept {
        x -= s;
        y -= s;
        z -= s;
        return *this;
    }
    constexpr Vec3& operator*=(float s) noexcept {
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }
    constexpr Vec3& operator/=(float s) noexcept {
        x /= s;
        y /= s;
        z /= s;
        return *this;
    }

    float At(int i) const {
        assert(i >= 0 && i <= 2 && "Vec3: Index out-of-bounds");
        return (*this)[i];
    }
};

// Binary ops
[[nodiscard]] constexpr Vec3 operator+(Vec3 a, const Vec3& b) noexcept { return a += b; }
[[nodiscard]] constexpr Vec3 operator-(Vec3 a, const Vec3& b) noexcept { return a -= b; }
[[nodiscard]] constexpr Vec3 operator*(Vec3 a, const Vec3& b) noexcept { return a *= b; }
[[nodiscard]] constexpr Vec3 operator/(Vec3 a, const Vec3& b) noexcept { return a /= b; }

[[nodiscard]] constexpr Vec3 operator*(Vec3 a, float v) noexcept { return a *= v; }
[[nodiscard]] constexpr Vec3 operator*(float v, Vec3 a) noexcept { return a *= v; }
[[nodiscard]] constexpr Vec3 operator/(Vec3 a, float v) noexcept { return a /= v; }

// Comparison ops
[[nodiscard]] constexpr bool operator==(const Vec3& a, const Vec3& b) noexcept {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
[[nodiscard]] constexpr bool operator<(const Vec3& a, float v) noexcept { return a.x < v && a.y < v && a.z < v; }
[[nodiscard]] constexpr bool operator<(const Vec3& a, const Vec3& b) noexcept {
    return a.x < b.x && a.y < b.y && a.z < b.z;
}

// Math utils
[[nodiscard]] inline float Length(const Vec3& v) noexcept { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }
[[nodiscard]] inline float Average(const Vec3& v) noexcept { return (v.x + v.y + v.z) / 3.0f; }
[[nodiscard]] inline Vec3 Normalize(Vec3 v, float eps = kEpsilon) noexcept {
    float len = Length(v);
    if (len <= eps) {
        return Vec3(0.0f);
    }
    return v /= len;
}
[[nodiscard]] constexpr float Dot(const Vec3& a, const Vec3& b) noexcept { return (a.x * b.x + a.y * b.y + a.z * b.z); }
[[nodiscard]] constexpr Vec3 Cross(const Vec3& a, const Vec3& b) noexcept {
    return Vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
[[nodiscard]] constexpr Vec3 Abs(const Vec3& v) noexcept {
    return Vec3(std::fabs(v.x), std::fabs(v.y), std::fabs(v.z));
}
[[nodiscard]] constexpr Vec3 Lerp(const Vec3& a, const Vec3& b, float t) noexcept { return a + (b - a) * t; }
[[nodiscard]] inline bool AlmostEqual(const Vec3& a, const Vec3& b, float eps = kEpsilon) noexcept {
    return std::fabs(a.x - b.x) <= eps && std::fabs(a.y - b.y) <= eps && std::fabs(a.z - b.z) <= eps;
}

inline std::ostream& operator<<(std::ostream& os, const Vec3& vec) {
    os << vec.x << " " << vec.y << " " << vec.z;
    return os;
}

static_assert(std::is_trivially_copyable_v<Vec3>);
static_assert(std::is_standard_layout_v<Vec3>);
static_assert(sizeof(Vec3) == 12);

#pragma once

#include <algorithm>
#include <limits>
#include <ostream>
#include <vector>

// Constants
const float kEpsilon = 1e-6;
const float kInfinity = std::numeric_limits<float>::max();

// Comparison
[[nodiscard]] inline bool AlmostEqual(float a, float b, float eps = kEpsilon) noexcept {
    return std::fabs(a - b) < eps;
}

// Hashing
[[nodiscard]] inline uint32_t HashU32(uint32_t x) {
    // Simple, fast avalanche hash (good enough for sampling)
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

[[nodiscard]] static inline float U01FromU32(uint32_t x) {
    // Use the top 24 bits -> exactly representable steps in float
    return (x >> 8) * (1.0f / 16777216.0f);  // 2^24
}

// Printing
template <typename T>
inline std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec) {
    os << "[ ";
    std::for_each(vec.begin(), vec.end(), [&os](const auto& e) { os << e << " "; });
    os << "]";
    return os;
}

template <typename... Args>
static void PrintFmt(std::ostream& os, std::format_string<Args...> fmt, Args&&... args) {
    os << std::format(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
static void PrintLnFmt(std::ostream& os, std::format_string<Args...> fmt, Args&&... args) {
    os << std::format(fmt, std::forward<Args>(args)...) << '\n';
}

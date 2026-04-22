#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <limits>
#include <ostream>
#include <string_view>
#include <vector>

namespace diplodocus {

// Constants
constexpr float kEpsilon = 1e-6;
constexpr float kInfinity = std::numeric_limits<float>::max();

// Comparison
[[nodiscard]] inline bool AlmostEqual(float a, float b, float eps = kEpsilon) noexcept {
    return std::fabs(a - b) < eps;
}

// Hashing
[[nodiscard]] inline float U01FromU32(uint32_t x) {
    // Use the top 24 bits -> flaot mantisa
    return (x >> 8) * (1.0f / 16777216.0f);  // 2^24
}

[[nodiscard]] inline uint32_t HashCombine(uint32_t h, uint32_t v) {
    // Avalanche hash
    v ^= v >> 16;
    v *= 0x7feb352d;  // MurmurHash3 constant
    v ^= v >> 15;
    v *= 0x846ca68b;  // MurmurHash3 constant
    v ^= v >> 16;

    h ^= v;
    h *= 0x9e3779b1;  // golden ratio constant

    return h;
}

[[nodiscard]] inline uint32_t HashU32(uint32_t x) {
    // Avalanche hash - MurmurHash3 finalizer
    x ^= x >> 16;
    x *= 0x85ebca6b;
    x ^= x >> 13;
    x *= 0xc2b2ae35;
    x ^= x >> 16;

    return x;
}

template <typename... Values>
[[nodiscard]] inline float HashValuesU01(uint32_t seed, Values&&... values) {
    uint32_t key = seed;
    for (const auto& v : {values...}) {
        key = HashCombine(key, static_cast<int32_t>(v));
    }

    return U01FromU32(HashU32(key));
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

// Enum parsing
template <typename Enum>
Enum ParseEnum(std::string_view) {
    static_assert(sizeof(Enum) == 0, "ParseEnum not implemented for this enum type");
}

}  // namespace diplodocus

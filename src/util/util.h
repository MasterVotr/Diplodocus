#pragma once

#include <algorithm>
#include <limits>
#include <ostream>
#include <vector>

// Constants
const float kEpsilon = 1e-3;
const float kInfinity = std::numeric_limits<float>::max();

// Printing
template <typename T>
inline std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec) {
    os << "[ ";
    std::for_each(vec.begin(), vec.end(), [&os](const auto& e) { os << e << " "; });
    os << "]";
    return os;
}

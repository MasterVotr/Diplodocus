#pragma once

#include "util/vec3.h"

namespace diplodocus {

namespace color {

constexpr Vec3 kBlack = Vec3{0.0f};
constexpr Vec3 kWhite = Vec3{1.0f};

constexpr Vec3 kGray = Vec3{0.5f};
constexpr Vec3 kLightGray = Vec3{0.75f};
constexpr Vec3 kDarkGray = Vec3{0.25f};

constexpr Vec3 kRed = Vec3{1.0f, 0.0f, 0.0f};
constexpr Vec3 kGreen = Vec3{0.0f, 1.0f, 0.0f};
constexpr Vec3 kBlue = Vec3{0.0f, 0.0f, 1.0f};

constexpr Vec3 kCyan = Vec3{0.0f, 1.0f, 1.0f};
constexpr Vec3 kMagenta = Vec3{1.0f, 0.0f, 1.0f};
constexpr Vec3 kYellow = Vec3{1.0f, 1.0f, 0.0f};

}  // namespace color

}  // namespace diplodocus

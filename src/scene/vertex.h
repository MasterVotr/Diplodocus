#pragma once

#include "util/vec3.h"

namespace diplodocus {

struct Vertex {
    Vec3 pos;
    Vec3 normal;

    inline bool operator==(const Vertex& other) const { return pos == other.pos && normal == other.normal; }
};

}  // namespace diplodocus

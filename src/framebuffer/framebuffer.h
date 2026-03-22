#pragma once

#include <vector>

#include "util/vec3.h"

namespace diplodocus {

class Framebuffer {
   public:
    Framebuffer(int width = 0, int height = 0);

    void SetPixel(int x, int y, const Vec3& c);
    Vec3 GetPixel(int x, int y) const;
    int GetWidth() const;
    int GetHeight() const;
    const std::vector<float>& GetData() const;

   private:
    int width_, height_;
    std::vector<float> data_;
};

}  // namespace diplodocus

#pragma once

#include <span>
#include <vector>

#include "util/colors.h"
#include "util/vec3.h"

namespace diplodocus {

class Framebuffer {
   public:
    Framebuffer(int width = 0, int height = 0);

    int GetWidth() const;
    int GetHeight() const;
    Vec3 GetPixel(int x, int y) const;
    std::span<const float> GetData() const;

    void Resize(int width, int height);
    void SetPixel(int x, int y, const Vec3& c);
    void Clear(const Vec3& color = color::kBlack);

   private:
    int width_, height_;
    std::vector<float> data_;

    int PixelIdx(int x, int y) const;
};

}  // namespace diplodocus

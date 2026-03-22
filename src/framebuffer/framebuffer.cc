#include "framebuffer/framebuffer.h"

#include <cassert>

namespace diplodocus {

Framebuffer::Framebuffer(int width, int height) : width_(width), height_(height), data_(width * height * 3) {}

void Framebuffer::SetPixel(int x, int y, const Vec3& c) {
    assert(x < width_ && x >= 0 && "Framebuffer::SetPixel: out-of-bounds");
    assert(y < width_ && y >= 0 && "Framebuffer::SetPixel: out-of-bounds");

    int color_idx = (y * width_ + x) * 3;
    data_[color_idx + 0] = c.x;
    data_[color_idx + 1] = c.y;
    data_[color_idx + 2] = c.z;
}

Vec3 Framebuffer::GetPixel(int x, int y) const {
    return {data_[y * width_ + x + 0], data_[y * width_ + x + 1], data_[y * width_ + x + 2]};
}

int Framebuffer::GetWidth() const { return width_; }

int Framebuffer::GetHeight() const { return height_; }

const std::vector<float>& Framebuffer::GetData() const { return data_; }

}  // namespace diplodocus

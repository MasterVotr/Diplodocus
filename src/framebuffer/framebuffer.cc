#include "framebuffer/framebuffer.h"

#include <cassert>
#include <cstring>
#include <span>

#include "util/vec3.h"

namespace diplodocus {

Framebuffer::Framebuffer(int width, int height) : width_(width), height_(height), data_(width * height * 3) {}

int Framebuffer::GetWidth() const { return width_; }

int Framebuffer::GetHeight() const { return height_; }

Vec3 Framebuffer::GetPixel(int x, int y) const {
    int pixel_idx = PixelIdx(x, y);
    return {data_[pixel_idx + 0], data_[pixel_idx + 1], data_[pixel_idx + 2]};
}

std::span<const float> Framebuffer::GetData() const { return std::span<const float>(data_.begin(), data_.size()); }

void Framebuffer::Resize(int width, int height) {
    width_ = width;
    height_ = height;
    data_.resize(width_ * height_ * 3);
}

void Framebuffer::SetPixel(int x, int y, const Vec3& c) {
    int pixel_idx = PixelIdx(x, y);
    data_[pixel_idx + 0] = c.x;
    data_[pixel_idx + 1] = c.y;
    data_[pixel_idx + 2] = c.z;
}

void Framebuffer::Clear(const Vec3& color) {
    const int pixels = width_ * height_;
    for (int i = 0; i < pixels; i++) {
        int pixel_idx = i * 3;
        data_[pixel_idx + 0] = color.x;
        data_[pixel_idx + 1] = color.y;
        data_[pixel_idx + 2] = color.z;
    }
}

int Framebuffer::PixelIdx(int x, int y) const {
    assert(x < width_ && x >= 0 && "Framebuffer: out-of-bounds");
    assert(y < height_ && y >= 0 && "Framebuffer: out-of-bounds");

    return (y * width_ + x) * 3;
}

}  // namespace diplodocus

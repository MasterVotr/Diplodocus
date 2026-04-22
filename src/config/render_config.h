#pragma once

#include "util/colors.h"
#include "util/vec3.h"

namespace diplodocus {

enum class RendererType { kCpu, kGpu };

template <>
inline RendererType ParseEnum<RendererType>(std::string_view sv) {
    if (sv == "cpu") return RendererType::kCpu;
    if (sv == "gpu") return RendererType::kGpu;
    throw std::runtime_error("Invalid SceneLoadFormat: " + std::string(sv));
}

struct RenderConfig {
    RendererType renderer_type = RendererType::kCpu;
    int width = 800;
    int height = 600;
    Vec3 background_color = color::kBlack;
    bool backface_culling = false;
    int max_depth = 4;
    int area_light_sample_cnt = 8;
    int pixel_sample_cnt = 4;
    int seed = 42;
    float gamma_correction = 1.0f;
};

}  // namespace diplodocus

#pragma once

#include "renderer/renderer.h"

namespace diplodocus {

class GpuRaytracer : public Renderer {
   public:
    RenderResult StartRender(const RenderConfig& render_config, const AccelerationStructureConfig& acceleration_config,
                             const Scene& scene, Framebuffer& framebuffer, Stats& stats) override;
    void Reset() override;
    void Cancel() override { cancelled_ = true; }
    float GetProgress() const override { return progress_; }
    inline const char* GetName() const override { return "GPU Raytracer"; }

   private:
    std::atomic<bool> cancelled_ = false;
    std::atomic<float> progress_ = 0.0f;
};

}  // namespace diplodocus

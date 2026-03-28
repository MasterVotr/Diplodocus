#pragma once

#include "config/render_config.h"
#include "framebuffer/framebuffer.h"
#include "scene/scene.h"
#include "stats/stats.h"

namespace diplodocus {

enum class RenderResult { kDone, kCanceled, kError };

class Renderer {
   public:
    virtual ~Renderer() = default;

    virtual RenderResult StartRender(const RenderConfig& render_config, const Scene& scene, Framebuffer& framebuffer,
                                     Stats& stats) = 0;
    virtual void Reset() = 0;
    virtual void Cancel() = 0;
    virtual float GetProgress() const = 0;
    virtual const char* GetName() const = 0;
};

}  // namespace diplodocus

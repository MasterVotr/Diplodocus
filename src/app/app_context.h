#pragma once

#include "config/config.h"
#include "framebuffer/framebuffer.h"
#include "scene/scene.h"
#include "stats/stats.h"

namespace diplodocus {

struct AppContext {
    Config config;
    Scene scene;
    Framebuffer framebuffer;
    Stats stats;
};

}  // namespace diplodocus

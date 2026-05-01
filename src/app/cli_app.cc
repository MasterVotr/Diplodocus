#include "app/cli_app.h"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <memory>

#include "config/render_config.h"
#include "io/config/json_config_loader.h"
#include "io/image/image_exporter_factory.h"
#include "io/scene/scene_loader_factory.h"
#include "io/stats/stats_exporter_factory.h"
#include "renderer/cpu_raytracer.h"
#include "renderer/gpu_renderer.h"
#include "scene/scene.h"
#include "scene/vertex.h"
#include "util/colors.h"
#include "util/logger.h"
#include "util/vec3.h"

namespace diplodocus {

namespace {

Vec3 CalculateCameraUp(const Vec3& from, const Vec3& to, const Vec3& arbitrary_up) {
    Vec3 forward = Normalize(from - to);
    Vec3 right = Normalize(Cross(arbitrary_up, forward));
    return Cross(forward, right);
}

Scene CreateTestScene1() {
    Logger::debug("Creating a test scene 1 (quad)");
    Scene test_scene{};

    // Materials
    Material m0;
    m0.diffuse = color::kRed;
    Material m1;
    m1.diffuse = color::kBlue;
    test_scene.AddMaterial(m0);
    test_scene.AddMaterial(m1);

    // Triangles
    Vertex vs[5];
    vs[0].pos = {0.0f, 0.0f, 0.0f};
    vs[1].pos = {1.0f, 0.0f, 0.0f};
    vs[2].pos = {0.0f, 1.0f, 0.0f};
    vs[3].pos = {1.0f, 1.0f, 0.0f};
    Triangle t0;
    t0.v0 = vs[0];
    t0.v1 = vs[3];
    t0.v2 = vs[2];
    t0.material_id = 0;
    Triangle t1;
    t1.v0 = vs[0];
    t1.v1 = vs[1];
    t1.v2 = vs[3];
    t1.material_id = 1;
    test_scene.AddTriangle(t0);
    test_scene.AddTriangle(t1);

    // Lights
    PointLight p;
    p.color = color::kWhite;
    p.pos = {0.5f, 1.5f, 1.0f};
    test_scene.AddPointLight(p);

    // Camera
    Camera& c = test_scene.GetCamera();
    c.pos = {0.5f, 0.5f, 2.0f};
    c.dir = {0.0f, 0.0f, -1.0f};
    c.up = {0.0f, 1.0f, 0.0f};
    assert(c.up == CalculateCameraUp(c.pos, c.pos + c.dir, c.up));
    c.far = 1e3;
    c.fov = 0.6;

    return test_scene;
}

Scene CreateTestScene2() {
    Logger::debug("Creating a test scene 2 (octahedron)");
    Scene test_scene{};

    // Materials
    Material red{};
    red.diffuse = color::kRed;
    Material blue{};
    blue.diffuse = color::kBlue;
    Material green{};
    green.diffuse = color::kGreen;
    Material yellow{};
    yellow.diffuse = color::kYellow;
    test_scene.AddMaterial(red);
    test_scene.AddMaterial(blue);
    test_scene.AddMaterial(green);
    test_scene.AddMaterial(yellow);

    // Triangles
    Vertex vs[6];
    vs[0].pos = {0.5f, 1.0f, 0.5f};
    vs[1].pos = {0.5f, 0.0f, 0.5f};
    vs[2].pos = {0.5f, 0.5f, 0.0f};
    vs[3].pos = {1.0f, 0.5f, 0.5f};
    vs[4].pos = {0.5f, 0.5f, 1.0f};
    vs[5].pos = {0.0f, 0.5f, 0.5f};
    Triangle t0{};
    t0.v0 = vs[0];
    t0.v1 = vs[4];
    t0.v2 = vs[3];
    t0.material_id = 3;
    test_scene.AddTriangle(t0);
    Triangle t1{};
    t1.v0 = vs[0];
    t1.v1 = vs[5];
    t1.v2 = vs[4];
    t1.material_id = 2;
    test_scene.AddTriangle(t1);
    Triangle t2{};
    t2.v0 = vs[0];
    t2.v1 = vs[2];
    t2.v2 = vs[5];
    t2.material_id = 1;
    test_scene.AddTriangle(t2);
    Triangle t3{};
    t3.v0 = vs[0];
    t3.v1 = vs[3];
    t3.v2 = vs[2];
    t3.material_id = 0;
    test_scene.AddTriangle(t3);
    Triangle t4{};
    t4.v0 = vs[1];
    t4.v1 = vs[3];
    t4.v2 = vs[4];
    t4.material_id = 3;
    test_scene.AddTriangle(t4);
    Triangle t5{};
    t5.v0 = vs[1];
    t5.v1 = vs[4];
    t5.v2 = vs[5];
    t5.material_id = 2;
    test_scene.AddTriangle(t5);
    Triangle t6{};
    t6.v0 = vs[1];
    t6.v1 = vs[2];
    t6.v2 = vs[5];
    t6.material_id = 1;
    test_scene.AddTriangle(t6);
    Triangle t7{};
    t7.v0 = vs[1];
    t7.v1 = vs[2];
    t7.v2 = vs[3];
    t7.material_id = 0;
    test_scene.AddTriangle(t7);

    // Lights
    PointLight p{};
    p.color = color::kWhite;
    p.pos = {1.5f, 1.5f, 1.5f};
    p.power = 1.0f;
    test_scene.AddPointLight(p);

    // Camera
    Vec3 c_target = Vec3(0.5f);
    Camera& c = test_scene.GetCamera();
    c.pos = {2.5f, 2.5f, 2.5f};
    c.dir = Normalize(c_target - c.pos);
    c.up = CalculateCameraUp(c.pos, c_target, {0.0f, 1.0f, 0.0f});
    c.far = 1e3;
    c.fov = 0.6;

    return test_scene;
}

}  // namespace

CliApp::CliApp(AppParameters app_params) : App(app_params) {
    Logger::debug("CliApp created");

    // Parse console arguments
    if (app_params_.app_console_args.count != 2) {
        Logger::error("Usage: ./app <config.json>");
        exit(EXIT_FAILURE);
    }
    std::filesystem::path config_path(app_params.app_console_args[1]);

    // Setup config
    auto config_loader_ = std::make_unique<JsonConfigLoader>();
    app_ctx_.config = config_loader_->Load(config_path).value();

    // Setup Renderers
    renderers_.insert({RendererType::kCpu, std::make_unique<CpuRaytracer>()});
    renderers_.insert({RendererType::kGpu, std::make_unique<GpuRenderer>()});
}

void CliApp::Run() {
    Logger::info("Running Diplodocus...");

    // Load scene
    auto scene_loader = CreateSceneLoader(app_ctx_.config.scene_load_config);
    app_ctx_.scene = std::move(scene_loader->Load(app_ctx_.config.scene_load_config).value());

    // Render image
    renderers_[app_ctx_.config.render_config.renderer_type]->StartRender(
        app_ctx_.config.render_config, app_ctx_.config.acceleration_structure_config, app_ctx_.scene,
        app_ctx_.framebuffer, app_ctx_.stats);

    // Export image
    auto image_exporter = CreateImageExporter(app_ctx_.config.image_export_config);
    image_exporter->Export(app_ctx_.config.image_export_config, app_ctx_.framebuffer);

    // Export stats
    auto stats_exporter = CreateStatsExporter(app_ctx_.config.stats_export_config);
    stats_exporter->ExportOne(app_ctx_.config, app_ctx_.stats);
}

}  // namespace diplodocus

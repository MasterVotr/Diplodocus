#include "scene/scene.h"

#include <span>
#include <utility>

#include "scene/triangle.h"

namespace diplodocus {

void Scene::ReserveTriangles(int n) { triangles_.reserve(n); }

void Scene::ReserverMaterials(int n) { materials_.reserve(n); }

void Scene::ReservePointsLights(int n) { point_lights_.reserve(n); }

void Scene::ReserveAreaLights(int n) { area_lights_.reserve(n); }

void Scene::AddTriangle(Triangle triangle) { triangles_.emplace_back(std::move(triangle)); }

void Scene::AddMaterial(Material material) { materials_.emplace_back(std::move(material)); }

void Scene::AddPointLight(PointLight point_light) { point_lights_.emplace_back(std::move(point_light)); }

void Scene::AddAreaLight(AreaLight area_light) { area_lights_.emplace_back(std::move(area_light)); }

Camera& Scene::GetCamera() { return camera_; }

std::span<const Triangle> Scene::Triangles() const {
    return std::span<const Triangle>(triangles_.begin(), triangles_.size());
}

std::span<const Material> Scene::Materials() const {
    return std::span<const Material>(materials_.begin(), materials_.size());
}

std::span<const PointLight> Scene::PointLights() const {
    return std::span<const PointLight>(point_lights_.begin(), point_lights_.size());
}

std::span<const AreaLight> Scene::AreaLights() const {
    return std::span<const AreaLight>(area_lights_.begin(), area_lights_.size());
}

}  // namespace diplodocus

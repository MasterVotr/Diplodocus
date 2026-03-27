#pragma once

#include <span>
#include <vector>

#include "scene/camera.h"
#include "scene/light.h"
#include "scene/material.h"
#include "scene/triangle.h"

namespace diplodocus {

class Scene {
   public:
    void ReserveTriangles(int n);
    void ReserverMaterials(int n);
    void ReservePointsLights(int n);
    void ReserveAreaLights(int n);

    void AddTriangle(Triangle triangle);
    void AddMaterial(Material material);
    void AddPointLight(PointLight point_light);
    void AddAreaLight(AreaLight area_light);

    Camera& GetCamera();
    const Camera& GetCamera() const;
    std::span<const Triangle> Triangles() const;
    std::span<const Material> Materials() const;
    std::span<const PointLight> PointLights() const;
    std::span<const AreaLight> AreaLights() const;

   private:
    Camera camera_;
    std::vector<Triangle> triangles_;
    std::vector<Material> materials_;
    std::vector<PointLight> point_lights_;
    std::vector<AreaLight> area_lights_;
};

}  // namespace diplodocus

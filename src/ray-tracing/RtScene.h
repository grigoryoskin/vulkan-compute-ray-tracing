#pragma once

#include <vector>
#include <iostream>
#include "GpuModels.h"
#include "Bvh.h"
#include "../utils/glm.h"
#include "../scene/mesh.h"
#include "../utils/RootDir.h"

namespace GpuModel
{
    std::vector<Triangle> getTriangles(const std::string &path, uint materialIndex)
    {
        Mesh mesh(path);

        std::vector<Triangle> triangles;
        int numTris = mesh.indices.size() / 3;

        for (int i = 0; i < numTris; i++)
        {
            int i0 = mesh.indices[3 * i];
            int i1 = mesh.indices[3 * i + 1];
            int i2 = mesh.indices[3 * i + 2];

            Triangle t{mesh.vertices[i0].pos, mesh.vertices[i1].pos, mesh.vertices[i2].pos, materialIndex};
            triangles.push_back(t);
        }
        return triangles;
    }

    /*
     * The scene to be ray traced. All objects are split into triangles and put into a common triangle array.
     */
    struct Scene
    {
        // triangles contain all triangles from all objects in the scene.
        std::vector<Triangle> triangles;
        std::vector<Sphere> spheres;
        std::vector<Material> materials;
        std::vector<Light> lights;
        std::vector<BvhNode> bvhNodes;

        Scene()
        {
            const std::string path_prefix = std::string(ROOT_DIR) + "resources/";

            Material gray{MaterialType::Lambertian, glm::vec3(0.3f, 0.3f, 0.3f)};
            Material red{MaterialType::Lambertian, glm::vec3(0.9f, 0.1f, 0.1f)};
            Material green{MaterialType::Lambertian, glm::vec3(0.1f, 0.9f, 0.1f)};
            Material whiteLight{MaterialType::LightSource, glm::vec3(2.0f, 2.0f, 2.0f)};
            Material metal{MaterialType::Metal, glm::vec3(1.0f, 1.0f, 1.0f)};
            Material glass{MaterialType::Glass, glm::vec3(1.0f, 1.0f, 1.0f)};

            materials.push_back(gray);
            materials.push_back(red);
            materials.push_back(green);
            materials.push_back(whiteLight);
            materials.push_back(metal);
            materials.push_back(glass);

            std::vector<Bvh::Object0> objects;

            std::vector<Triangle> floorTriangles = getTriangles(path_prefix + "/models/doge_scene/floor.obj", 0);

            std::vector<Triangle> dogeTriangles = getTriangles(path_prefix + "/models/doge_scene/buff-doge.obj", 0);
            //std::vector<Triangle> dogeTriangles = getTriangles(path_prefix + "/models/doge_scene/box1.obj", 4);

            //std::vector<Triangle> cheemsTriangles = getTriangles(path_prefix + "/models/doge_scene/box2.obj", 0);
            std::vector<Triangle> cheemsTriangles = getTriangles(path_prefix + "/models/doge_scene/cheems.obj", 0);

            std::vector<Triangle> rightWallTriangles = getTriangles(path_prefix + "/models/doge_scene/right.obj", 1);
            std::vector<Triangle> leftWallTriangles = getTriangles(path_prefix + "/models/doge_scene/left.obj", 2);
            std::vector<Triangle> backWallTriangles = getTriangles(path_prefix + "/models/doge_scene/back.obj", 0);
            std::vector<Triangle> ceilTriangles = getTriangles(path_prefix + "/models/doge_scene/ceil.obj", 0);
            std::vector<Triangle> lightTriangles = getTriangles(path_prefix + "/models/doge_scene/light.obj", 3);

            triangles.insert(triangles.end(), dogeTriangles.begin(), dogeTriangles.end());
            triangles.insert(triangles.end(), cheemsTriangles.begin(), cheemsTriangles.end());
            triangles.insert(triangles.end(), rightWallTriangles.begin(), rightWallTriangles.end());
            triangles.insert(triangles.end(), leftWallTriangles.begin(), leftWallTriangles.end());
            triangles.insert(triangles.end(), backWallTriangles.begin(), backWallTriangles.end());
            triangles.insert(triangles.end(), ceilTriangles.begin(), ceilTriangles.end());
            triangles.insert(triangles.end(), floorTriangles.begin(), floorTriangles.end());
            triangles.insert(triangles.end(), lightTriangles.begin(), lightTriangles.end());

            for (uint32_t i = 0; i < triangles.size(); i++)
            {
                Triangle t = triangles[i];
                objects.push_back({i, t});
                if (materials[t.materialIndex].type == MaterialType::LightSource)
                {
                    float area = glm::length(glm::cross(t.v0, t.v1)) * 0.5f;
                    lights.push_back({i, area});
                }
            }

            spheres.push_back({glm::vec4(0.6, 1, -1, 0.6), 5});

            bvhNodes = Bvh::createBvh(objects);
        }
    };
}

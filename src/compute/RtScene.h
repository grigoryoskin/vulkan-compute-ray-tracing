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
    std::vector<Triangle> getTriangles(Mesh &mesh, uint materialIndex)
    {
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
        std::vector<Triangle> triangles;
        std::vector<Material> materials;
        std::vector<Light> lights;
        std::vector<BvhNode> bvhNodes;

        Scene(std::string path_prefix)
        {
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
            Mesh floor(path_prefix + "/models/doge_scene/floor.obj");
            std::vector<Triangle> floorTriangles = getTriangles(floor, 0);
            //Mesh doge(path_prefix + "/models/doge_scene/buff-doge.obj");
            Mesh doge(path_prefix + "/models/doge_scene/box1.obj");
            std::vector<Triangle> dogeTriangles = getTriangles(doge, 4);
            //Mesh cheems(path_prefix + "/models/doge_scene/cheems.obj");
            Mesh cheems(path_prefix + "/models/doge_scene/box2.obj");
            std::vector<Triangle> cheemsTriangles = getTriangles(cheems, 0);
            Mesh rightWall(path_prefix + "/models/doge_scene/right.obj");
            std::vector<Triangle> rightWallTriangles = getTriangles(rightWall, 1);
            Mesh leftWall(path_prefix + "/models/doge_scene/left.obj");
            std::vector<Triangle> leftWallTriangles = getTriangles(leftWall, 2);
            Mesh backWall(path_prefix + "/models/doge_scene/back.obj");
            std::vector<Triangle> backWallTriangles = getTriangles(backWall, 0);
            Mesh ceil(path_prefix + "/models/doge_scene/ceil.obj");
            std::vector<Triangle> ceilTriangles = getTriangles(ceil, 0);
            Mesh light(path_prefix + "/models/doge_scene/light.obj");
            std::vector<Triangle> lightTriangles = getTriangles(light, 3);

            triangles.insert(triangles.end(), dogeTriangles.begin(), dogeTriangles.end());
            triangles.insert(triangles.end(), cheemsTriangles.begin(), cheemsTriangles.end());
            triangles.insert(triangles.end(), rightWallTriangles.begin(), rightWallTriangles.end());
            triangles.insert(triangles.end(), leftWallTriangles.begin(), leftWallTriangles.end());
            triangles.insert(triangles.end(), backWallTriangles.begin(), backWallTriangles.end());
            triangles.insert(triangles.end(), ceilTriangles.begin(), ceilTriangles.end());
            triangles.insert(triangles.end(), floorTriangles.begin(), floorTriangles.end());
            triangles.insert(triangles.end(), lightTriangles.begin(), lightTriangles.end());

            std::cout << "num triangles " << triangles.size() << std::endl;

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

            std::cout << "num lights " << lights.size() << std::endl;

            std::vector<Bvh::BvhNode0> nodes0 = Bvh::createBvh(objects);
            bvhNodes = createGpuBvh(nodes0);
        }
    };
}

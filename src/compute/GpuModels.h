#pragma once

#include <vector>
#include <stack>
#include <stdlib.h>     /* srand, rand */
#include <algorithm>
#include "../utils/glm.h"

/**
 * Geometry and material objects to be used on GPU. To minimize data size integer links are used.
 */
namespace GpuModel {
    struct Material {
        alignas(4) bool emits;
        alignas(16) glm::vec3 albedo;
    };
    
    struct Triangle {
        alignas(16) glm::vec3 v0;
        alignas(16) glm::vec3 v1;
        alignas(16) glm::vec3 v2;
        alignas(4)  uint materialIndex;
    };

    // Node in a non recursive BHV for use on GPU.
    struct BvhNode {
        alignas(16) glm::vec3 min;
        alignas(16) glm::vec3 max;
        alignas(4) int leftNodeIndex = -1;
        alignas(4) int rightNodeIndex = -1;
        alignas(4) int objectIndex = -1;
    };

    // Model of light used for importance sampling.
    struct Light {
        // Index in the array of triangles.
        alignas(4) uint triangleIndex;
        // Area of the triangle;
        alignas(4) float area;
    };
}

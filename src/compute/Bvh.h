#pragma once

#include <vector>
#include <stack>
#include <stdlib.h> /* srand, rand */
#include <algorithm>
#include "../utils/glm.h"
#include "GpuModels.h"

/**
 * Functions and structs for bvh construction. 
 * Since GPU doesn't support recursion, non-recursive structures are used as much as possible. 
 */
namespace Bvh
{
    const glm::vec3 eps(0.00001);

    // Axis-aligned bounding box.
    struct Aabb
    {
        alignas(16) glm::vec3 min;
        alignas(16) glm::vec3 max;

        int longestAxis()
        {
            float x = abs(max[0] - min[0]);
            float y = abs(max[1] - min[1]);
            float z = abs(max[2] - min[2]);

            int longest = 0;
            if (y > x && y > z)
            {
                longest = 1;
            }
            if (z > x && z > y)
            {
                longest = 2;
            }
            return longest;
        }

        int randomAxis()
        {
            return rand() % 3;
        }
    };

    // Utility structure to keep track of the initial triangle index in the triangles array while sorting.
    struct Object0
    {
        uint32_t index;
        GpuModel::Triangle t;
    };

    struct BvhNode0
    {
        Aabb box;
        // index refers to the index in the final array of nodes.
        int index = -1;
        int leftNodeIndex = -1;
        int rightNodeIndex = -1;
        std::vector<Object0> objects;
    };

    Aabb surroundingBox(Aabb box0, Aabb box1)
    {
        glm::vec3 small(fmin(box0.min.x, box1.min.x),
                        fmin(box0.min.y, box1.min.y),
                        fmin(box0.min.z, box1.min.z));

        glm::vec3 big(fmax(box0.max.x, box1.max.x),
                      fmax(box0.max.y, box1.max.y),
                      fmax(box0.max.z, box1.max.z));

        return {small, big};
    }

    Aabb objectBoundingBox(GpuModel::Triangle &t)
    {
        glm::vec3 small(fmin(fmin(t.v0.x, t.v1.x), t.v2.x),
                        fmin(fmin(t.v0.y, t.v1.y), t.v2.y),
                        fmin(fmin(t.v0.z, t.v1.z), t.v2.z));

        glm::vec3 big(fmax(fmax(t.v0.x, t.v1.x), t.v2.x),
                      fmax(fmax(t.v0.y, t.v1.y), t.v2.y),
                      fmax(fmax(t.v0.z, t.v1.z), t.v2.z));

        return {small - eps, big + eps};
    }

    Aabb objectListBoundingBox(std::vector<Object0> &objects)
    {
        Aabb tempBox;
        Aabb outputBox;
        bool firstBox = true;

        for (auto &object : objects)
        {
            tempBox = objectBoundingBox(object.t);
            outputBox = firstBox ? tempBox : surroundingBox(outputBox, tempBox);
            firstBox = false;
        }

        return outputBox;
    }

    inline bool boxCompare(GpuModel::Triangle &a, GpuModel::Triangle &b, int axis)
    {
        Aabb boxA = objectBoundingBox(a);
        Aabb boxB = objectBoundingBox(b);

        return boxA.min[axis] < boxB.min[axis];
    }

    bool boxXCompare(Object0 a, Object0 b)
    {
        return boxCompare(a.t, b.t, 0);
    }

    bool boxYCompare(Object0 a, Object0 b)
    {
        return boxCompare(a.t, b.t, 1);
    }

    bool boxZCompare(Object0 a, Object0 b)
    {
        return boxCompare(a.t, b.t, 2);
    }

    bool nodeCompare(BvhNode0 &a, BvhNode0 &b)
    {
        return a.index < b.index;
    }

    // Works only for triangles, no spheres yet.
// TODO: extend for spheres.
    std::vector<BvhNode0> createBvh(const std::vector<Object0> &srcObjects)
    {
        std::vector<BvhNode0> output;
        int nodeCounter = 0;
        std::stack<BvhNode0> nodeStack;
        BvhNode0 root;
        root.index = nodeCounter;
        root.objects = srcObjects;
        nodeCounter++;
        nodeStack.push(root);

        while (!nodeStack.empty())
        {
            BvhNode0 currentNode = nodeStack.top();
            nodeStack.pop();

            currentNode.box = objectListBoundingBox(currentNode.objects);

            int axis = currentNode.box.randomAxis();
            auto comparator = (axis == 0)   ? boxXCompare
                              : (axis == 1) ? boxYCompare
                                            : boxZCompare;

            size_t objectSpan = currentNode.objects.size();
            std::sort(currentNode.objects.begin(), currentNode.objects.end(), comparator);

            if (objectSpan == 1)
            {
                output.push_back(currentNode);
                continue;
            }
            else
            {
                auto mid = objectSpan / 2;

                BvhNode0 leftNode;
                leftNode.index = nodeCounter;
                for (int i = 0; i < mid; i++)
                {
                    leftNode.objects.push_back(currentNode.objects[i]);
                }
                nodeCounter++;
                nodeStack.push(leftNode);

                BvhNode0 rightNode;
                rightNode.index = nodeCounter;
                for (int i = mid; i < currentNode.objects.size(); i++)
                {
                    rightNode.objects.push_back(currentNode.objects[i]);
                }
                nodeCounter++;
                nodeStack.push(rightNode);

                currentNode.leftNodeIndex = leftNode.index;
                currentNode.rightNodeIndex = rightNode.index;
                output.push_back(currentNode);
            }
        }
        std::sort(output.begin(), output.end(), nodeCompare);

        return output;
    }

    // Converting bhv to gpu model;
    std::vector<GpuModel::BvhNode> createGpuBvh(std::vector<BvhNode0> &inputNodes)
    {
        std::vector<GpuModel::BvhNode> outputNodes;
        for (int i = 0; i < inputNodes.size(); i++)
        {
            BvhNode0 iNode = inputNodes[i];
            int rightChild = iNode.rightNodeIndex;
            int leftChild = iNode.leftNodeIndex;
            bool leaf = rightChild == -1 || leftChild == -1;

            GpuModel::BvhNode node;
            node.min = iNode.box.min;
            node.max = iNode.box.max;
            node.leftNodeIndex = leftChild;
            node.rightNodeIndex = rightChild;

            if (leaf)
            {
                node.objectIndex = iNode.objects[0].index;
            }

            outputNodes.push_back(node);
        }

        //createLinks(inputNodes, outputNodes, -1, 0);
        return outputNodes;
    }
}
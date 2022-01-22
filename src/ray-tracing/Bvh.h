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
    const glm::vec3 eps(0.0001);

    // Axis-aligned bounding box.
    struct Aabb
    {
        alignas(16) glm::vec3 min = {FLT_MAX, FLT_MAX, FLT_MAX};
        alignas(16) glm::vec3 max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

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

    // Intermediate BvhNode structure needed for constructing Bvh.
    struct BvhNode0
    {
        Aabb box;
        // index refers to the index in the final array of nodes. Used for sorting a flattened Bvh.
        int index = -1;
        int leftNodeIndex = -1;
        int rightNodeIndex = -1;
        std::vector<Object0> objects;

        GpuModel::BvhNode getGpuModel()
        {
            bool leaf = leftNodeIndex == -1 && rightNodeIndex == -1;

            GpuModel::BvhNode node;
            node.min = box.min;
            node.max = box.max;
            node.leftNodeIndex = leftNodeIndex;
            node.rightNodeIndex = rightNodeIndex;

            if (leaf)
            {
                node.objectIndex = objects[0].index;
            }

            return node;
        }
    };

    bool nodeCompare(BvhNode0 &a, BvhNode0 &b)
    {
        return a.index < b.index;
    }

    Aabb surroundingBox(Aabb box0, Aabb box1)
    {
        return {glm::min(box0.min, box1.min), glm::max(box0.max, box1.max)};
    }

    Aabb objectBoundingBox(GpuModel::Triangle &t)
    {
        // Need to add eps to correctly construct an AABB for flat objects like planes.
        return {glm::min(glm::min(t.v0, t.v1), t.v2) - eps, glm::max(glm::max(t.v0, t.v1), t.v2) + eps};
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

    // Since GPU can't deal with tree structures we need to create a flattened BVH.
    // Stack is used instead of a tree.
    std::vector<GpuModel::BvhNode> createBvh(const std::vector<Object0> &srcObjects)
    {
        std::vector<BvhNode0> intermediate;
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

            if (objectSpan <= 1)
            {
                intermediate.push_back(currentNode);
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
                for (int i = mid; i < objectSpan; i++)
                {
                    rightNode.objects.push_back(currentNode.objects[i]);
                }
                nodeCounter++;
                nodeStack.push(rightNode);

                currentNode.leftNodeIndex = leftNode.index;
                currentNode.rightNodeIndex = rightNode.index;
                intermediate.push_back(currentNode);
            }
        }
        std::sort(intermediate.begin(), intermediate.end(), nodeCompare);

        std::vector<GpuModel::BvhNode> output;
        output.reserve(intermediate.size());
        for (int i = 0; i < intermediate.size(); i++)
        {
            output.push_back(intermediate[i].getGpuModel());
        }
        return output;
    }
}

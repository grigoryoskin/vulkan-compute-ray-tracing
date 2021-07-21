#pragma once

#include "../utils/vulkan.h"
#include "vk_mem_alloc.h"
#include <string>
#include <vector>
#include "../app-context/VulkanApplicationContext.h"
#include "../scene/mesh.h"

namespace VulkanMemory
{
    template <typename T>
    struct VulkanBuffer
    {
        VkBuffer buffer;
        VmaAllocation allocation;

        void create(T *elements, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
        void create(std::vector<T> &elements, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
        void allocate(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
        void destroy();
    };
}

#include "VulkanBuffer.tpp"
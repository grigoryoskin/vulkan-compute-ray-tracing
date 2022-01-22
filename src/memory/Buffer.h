#pragma once

#include "../utils/vulkan.h"
#include "vk_mem_alloc.h"
#include <string>
#include <iostream>
#include <vector>
#include "../app-context/VulkanApplicationContext.h"
#include "../scene/Mesh.h"

namespace mcvkp
{
    struct Buffer
    {
        VkBuffer buffer;
        VmaAllocation allocation;
        VkDeviceSize size;

        ~Buffer()
        {
            std::cout << "Destroying buffer"
                      << "\n";
            if (buffer != VK_NULL_HANDLE)
            {
                vmaDestroyBuffer(VulkanGlobal::context.allocator, buffer, allocation);
                buffer = VK_NULL_HANDLE;
            }
        }

        VkDescriptorBufferInfo getDescriptorInfo()
        {
            VkDescriptorBufferInfo descriptorInfo{};
            descriptorInfo.buffer = buffer;
            descriptorInfo.offset = 0;
            descriptorInfo.range = size;
            return descriptorInfo;
        }
    };

    struct BufferBundle
    {
        std::vector<std::shared_ptr<Buffer> > buffers;

        BufferBundle(size_t numBuffers)
        {
            for (size_t i = 0; i < numBuffers; i++)
            {
                buffers.push_back(std::make_shared<Buffer>());
            }
        }
    };

    namespace BufferUtils
    {
        void inline allocate(Buffer *buffer,
                             VkDeviceSize size,
                             VkBufferUsageFlags usage,
                             VmaMemoryUsage memoryUsage)
        {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = size;
            bufferInfo.usage = usage;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo vmaallocInfo = {};
            vmaallocInfo.usage = memoryUsage;

            if (vmaCreateBuffer(VulkanGlobal::context.allocator,
                                &bufferInfo,
                                &vmaallocInfo,
                                &buffer->buffer,
                                &buffer->allocation,
                                nullptr) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create buffer");
            }
        }

        template <typename T>
        void inline create(Buffer *buffer, const T *elements, const size_t numElements, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
        {
            buffer->size = sizeof(T);

            allocate(buffer, numElements * sizeof(T), usage, memoryUsage);

            void *data;
            vmaMapMemory(VulkanGlobal::context.allocator, buffer->allocation, &data);
            memcpy(data, elements, numElements * sizeof(T));
            vmaUnmapMemory(VulkanGlobal::context.allocator, buffer->allocation);
        }

        template <typename T>
        void inline createBundle(BufferBundle *bufferBundle, const T *elements, const size_t numElements, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
        {
            for (auto &buffer : bufferBundle->buffers)
            {
                create(buffer.get(), elements, numElements, usage, memoryUsage);
            }
        }

        template <typename T>
        void inline createBundle(BufferBundle *bufferBundle, const T &element, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
        {
            createBundle(bufferBundle, &element, 1, usage, memoryUsage);
        }
    };
}

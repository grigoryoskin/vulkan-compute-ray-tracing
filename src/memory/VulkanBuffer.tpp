#include "../utils/vulkan.h"
#include "vk_mem_alloc.h"
#include <string>
#include <vector>
#include "../app-context/VulkanApplicationContext.h"

namespace VulkanMemory
{
    template <typename T>
    void VulkanBuffer<T>::destroy()
    {
        vmaDestroyBuffer(VulkanGlobal::context.allocator, buffer, allocation);
    }

    template <typename T>
    void VulkanBuffer<T>::allocate(VkDeviceSize size,
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
                            &buffer,
                            &allocation,
                            nullptr) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create buffer");
        }
    }

    template <typename T>
    void VulkanBuffer<T>::create(T *elements, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
    {
        allocate(size, usage, memoryUsage);

        void *data;
        vmaMapMemory(VulkanGlobal::context.allocator, allocation, &data);
        memcpy(data, elements, size);
        vmaUnmapMemory(VulkanGlobal::context.allocator, allocation);
    }

    template <typename T>
    void VulkanBuffer<T>::create(std::vector<T> &elements, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
    {
        VkDeviceSize bufferSize = sizeof(T) * elements.size();
        create(elements.data(), bufferSize, usage, memoryUsage);
    }
}

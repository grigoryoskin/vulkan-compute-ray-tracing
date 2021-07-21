#pragma once

#include <vector>
#include "../utils/vulkan.h"
#include "mesh.h"
#include "../memory/VulkanBuffer.h"

class DrawableModel
{
public:
    Mesh mesh;
    VulkanMemory::VulkanBuffer<Vertex> vertexBuffer;
    VulkanMemory::VulkanBuffer<uint32_t> indexBuffer;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    void drawCommand(VkCommandBuffer &commandBuffer, VkPipelineLayout &pipelineLayout, size_t currentFrame)
    {
        VkBuffer vertexBuffers[] = {vertexBuffer.buffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
    }

protected:
    VkDescriptorSetLayout *descriptorSetLayout;

    void initVertexBuffer()
    {
        vertexBuffer.create(mesh.vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    }

    void initIndexBuffer()
    {
        indexBuffer.create(mesh.indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    }
};

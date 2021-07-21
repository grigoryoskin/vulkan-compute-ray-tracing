#pragma once

#include <array>
#include <string>
#include <vector>
#include <stack>
#include <stdlib.h> /* srand, rand */
#include <algorithm>
#include "../utils/vulkan.h"
#include "../memory/VulkanBuffer.h"
#include "../memory/VulkanImage.h"
#include "../pipeline/VulkanDescriptorSet.h"
#include "../pipeline/VulkanPipeline.h"
#include "../utils/glm.h"
#include "Bvh.h"
#include "RtScene.h"

struct UniformBufferObject
{
    glm::vec3 camPosition;
    float time;
    u_int32_t currentSample;
};

/*
 * Wrapper for compute pipeline, decriptor sets, render target texture and model buffers.
 */
class TextureOutputComputeModel
{
public:
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    VulkanImage::VulkanImage targetTexture;
    VkSampler textureSampler;
    std::vector<VulkanMemory::VulkanBuffer<UniformBufferObject> > uniformBuffers;

    VulkanMemory::VulkanBuffer<GpuModel::Triangle> triangleBuffer;
    VulkanMemory::VulkanBuffer<GpuModel::Material> materialBuffer;
    VulkanMemory::VulkanBuffer<GpuModel::BvhNode> bvhNodeBuffer;

    GpuModel::Scene *scene;
    void init(VulkanSwapchain &swapchainContext)
    {
        const std::string path_prefix = std::string(ROOT_DIR) + "resources/";

        VulkanDescriptorSet::computeStorageImageLayout(descriptorSetLayout);
        VulkanPipeline::createComputePipeline(&descriptorSetLayout,
                                              path_prefix + "/shaders/generated/ray-trace-compute.spv",
                                              pipelineLayout,
                                              pipeline);

        // Initializing the render target texture.
        VulkanImage::createImage(swapchainContext.swapChainExtent.width,
                                 swapchainContext.swapChainExtent.height,
                                 1,
                                 VK_SAMPLE_COUNT_1_BIT,
                                 VK_FORMAT_R8G8B8A8_UNORM,
                                 VK_IMAGE_TILING_OPTIMAL,
                                 VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
                                 VK_IMAGE_ASPECT_COLOR_BIT,
                                 VMA_MEMORY_USAGE_GPU_ONLY,
                                 targetTexture);

        VulkanImage::transitionImageLayout(targetTexture.image,
                                           VK_FORMAT_R8G8B8A8_UNORM,
                                           VK_IMAGE_LAYOUT_UNDEFINED,
                                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                           1);

        uint32_t mips = 1;
        VulkanImage::createTextureSampler(textureSampler, mips);

        uint32_t descriptorSetsSize = static_cast<uint32_t>(swapchainContext.swapChainImages.size());

        initUniformBuffers(descriptorSetsSize);
        initSceneBuffers(path_prefix);
        initDescriptorPool(descriptorSetsSize);
        initDescriptorSets(descriptorSetsSize);
    }

    void destroy()
    {
        vkDestroyDescriptorSetLayout(VulkanGlobal::context.device, descriptorSetLayout, nullptr);
        vkDestroyPipeline(VulkanGlobal::context.device, pipeline, nullptr);
        vkDestroyPipelineLayout(VulkanGlobal::context.device, pipelineLayout, nullptr);
        for (size_t i = 0; i < uniformBuffers.size(); i++)
        {
            uniformBuffers[i].destroy();
        }
        targetTexture.destroy();
        triangleBuffer.destroy();
        materialBuffer.destroy();
        bvhNodeBuffer.destroy();
        vkDestroySampler(VulkanGlobal::context.device, textureSampler, nullptr);
        vkDestroyDescriptorPool(VulkanGlobal::context.device, descriptorPool, nullptr);
    }

    void updateUniformBuffer(UniformBufferObject &ubo, uint32_t currentImage)
    {
        VkDeviceSize bufferSize = sizeof(ubo);

        void *data;
        vmaMapMemory(VulkanGlobal::context.allocator, uniformBuffers[currentImage].allocation, &data);
        memcpy(data, &ubo, bufferSize);
        vmaUnmapMemory(VulkanGlobal::context.allocator, uniformBuffers[currentImage].allocation);
    }

private:
    void initUniformBuffers(uint32_t descriptorSetsSize)
    {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);
        uniformBuffers.resize(descriptorSetsSize);

        for (size_t i = 0; i < descriptorSetsSize; i++)
        {
            uniformBuffers[i].allocate(bufferSize,
                                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                       VMA_MEMORY_USAGE_CPU_TO_GPU);
        }
    }

    void initSceneBuffers(std::string path_prefix)
    {
        scene = new GpuModel::Scene(path_prefix);

        triangleBuffer.create(scene->triangles, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        materialBuffer.create(scene->materials, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        bvhNodeBuffer.create(scene->bvhNodes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    }

    void initDescriptorPool(uint32_t descriptorSetsSize)
    {
        std::array<VkDescriptorPoolSize, 5> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        poolSizes[0].descriptorCount = descriptorSetsSize;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[1].descriptorCount = descriptorSetsSize;
        poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[2].descriptorCount = descriptorSetsSize;
        poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[3].descriptorCount = descriptorSetsSize;
        poolSizes[4].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[4].descriptorCount = descriptorSetsSize;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = descriptorSetsSize;

        if (vkCreateDescriptorPool(VulkanGlobal::context.device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void initDescriptorSets(uint32_t descriptorSetsSize)
    {
        std::vector<VkDescriptorSetLayout> layouts(descriptorSetsSize, descriptorSetLayout);

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = descriptorSetsSize;
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(descriptorSetsSize);
        if (vkAllocateDescriptorSets(VulkanGlobal::context.device, &allocInfo, descriptorSets.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < descriptorSetsSize; i++)
        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i].buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            imageInfo.imageView = targetTexture.imageView;
            imageInfo.sampler = textureSampler;

            VkDescriptorBufferInfo triangleBufferInfo{};
            triangleBufferInfo.buffer = triangleBuffer.buffer;
            triangleBufferInfo.offset = 0;
            triangleBufferInfo.range = sizeof(GpuModel::Triangle) * scene->triangles.size();

            VkDescriptorBufferInfo materialBufferInfo{};
            materialBufferInfo.buffer = materialBuffer.buffer;
            materialBufferInfo.offset = 0;
            materialBufferInfo.range = sizeof(GpuModel::Material) * scene->materials.size();

            VkDescriptorBufferInfo bvhNodeBufferInfo{};
            bvhNodeBufferInfo.buffer = bvhNodeBuffer.buffer;
            bvhNodeBufferInfo.offset = 0;
            bvhNodeBufferInfo.range = sizeof(GpuModel::BvhNode) * scene->bvhNodes.size();

            std::array<VkWriteDescriptorSet, 5> descriptorWrites{};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pImageInfo = &imageInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pBufferInfo = &bufferInfo;

            descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[2].dstSet = descriptorSets[i];
            descriptorWrites[2].dstBinding = 2;
            descriptorWrites[2].dstArrayElement = 0;
            descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrites[2].descriptorCount = 1;
            descriptorWrites[2].pBufferInfo = &triangleBufferInfo;

            descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[3].dstSet = descriptorSets[i];
            descriptorWrites[3].dstBinding = 3;
            descriptorWrites[3].dstArrayElement = 0;
            descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrites[3].descriptorCount = 1;
            descriptorWrites[3].pBufferInfo = &materialBufferInfo;

            descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[4].dstSet = descriptorSets[i];
            descriptorWrites[4].dstBinding = 4;
            descriptorWrites[4].dstArrayElement = 0;
            descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrites[4].descriptorCount = 1;
            descriptorWrites[4].pBufferInfo = &bvhNodeBufferInfo;

            vkUpdateDescriptorSets(VulkanGlobal::context.device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
        }
    }
};

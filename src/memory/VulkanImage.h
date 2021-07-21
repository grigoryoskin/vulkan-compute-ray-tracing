#pragma once
#include "../utils/vulkan.h"
#include "../app-context/VulkanApplicationContext.h"
#include <iostream>
#include <string>
#include "vk_mem_alloc.h"

namespace VulkanImage
{
    struct VulkanImage
    {
        VkImage image;
        VmaAllocation allocation;
        VkImageView imageView;
        uint32_t width;
        uint32_t height;

        void destroy();
    };

    VkImageView createImageView(VkImage &image,
                                VkFormat format,
                                VkImageAspectFlags aspectFlags,
                                uint32_t mipLevels);

    void createImage(uint32_t width,
                     uint32_t height,
                     uint32_t mipLevels,
                     VkSampleCountFlagBits numSamples,
                     VkFormat format,
                     VkImageTiling tiling,
                     VkImageUsageFlags usage,
                     VkImageAspectFlags aspectFlags,
                     VmaMemoryUsage memoryUsage,
                     VulkanImage &allocatedImage);

    void transitionImageLayout(VkImage image,
                               VkFormat format,
                               VkImageLayout oldLayout,
                               VkImageLayout newLayout,
                               uint32_t mipLevels);

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    void generateMipmaps(VkImage image,
                         VkFormat imageFormat,
                         int32_t texWidth,
                         int32_t texHeight,
                         uint32_t mipLevels);

    void createTextureImage(std::string path,
                            VulkanImage &allocatedImage,
                            uint32_t &mipLevels);

    void createTextureSampler(VkSampler &textureSampler, uint32_t &mipLevels);

    void cmdBlitTexture(VkCommandBuffer &commandBuffer, VulkanImage &srcImage, VulkanImage &dstImage);
}

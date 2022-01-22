#pragma once
#include "../utils/vulkan.h"
#include "../app-context/VulkanApplicationContext.h"
#include <iostream>
#include <memory>
#include <string>
#include "vk_mem_alloc.h"

namespace mcvkp
{
    class Image
    {
    public:
        VkImage image;
        VmaAllocation allocation;
        VkImageView imageView;
        uint32_t width;
        uint32_t height;

        ~Image();
        void destroy();
        VkDescriptorImageInfo getDescriptorInfo(VkImageLayout imageLayout);
    };

    namespace ImageUtils
    {
        VkImageView createImageView(VkImage &image,
                                    VkFormat format,
                                    VkImageAspectFlags aspectFlags,
                                    const uint32_t &mipLevels);

        void createImage(uint32_t width,
                         uint32_t height,
                         uint32_t mipLevels,
                         VkSampleCountFlagBits numSamples,
                         VkFormat format,
                         VkImageTiling tiling,
                         VkImageUsageFlags usage,
                         VkImageAspectFlags aspectFlags,
                         VmaMemoryUsage memoryUsage,
                         std::shared_ptr<Image> allocatedImage);

        void transitionImageLayout(VkImage image,
                                   VkFormat format,
                                   VkImageLayout oldLayout,
                                   VkImageLayout newLayout,
                                   const uint32_t &mipLevels);

        void copyBufferToImage(const VkBuffer &buffer, VkImage image, uint32_t width, uint32_t height);

        void generateMipmaps(VkImage image,
                             VkFormat imageFormat,
                             int32_t texWidth,
                             int32_t texHeight,
                             const uint32_t &mipLevels);

        void createTextureImage(const std::string &path,
                                Image &allocatedImage,
                                uint32_t &mipLevels);

        void createTextureSampler(std::shared_ptr<VkSampler> textureSampler, uint32_t &mipLevels);
    }

    class Texture
    {
    public:
        Texture(const std::string &path);
        Texture(const std::shared_ptr<Image> &image);

        ~Texture();

        std::shared_ptr<Image> getImage() { return m_image; }
        std::shared_ptr<VkSampler> getSampler() { return m_sampler; }
        VkDescriptorImageInfo getDescriptorInfo();

    private:
        std::shared_ptr<Image> m_image;
        std::shared_ptr<VkSampler> m_sampler;
        uint32_t m_mips = 1;
    };
}

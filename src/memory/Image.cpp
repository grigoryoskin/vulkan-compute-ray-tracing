#include <iostream>
#include <string>
#include <cmath>
#include "vk_mem_alloc.h"
#include "Buffer.h"
#include "../render-context/RenderSystem.h"
#include "../utils/StbImageImpl.h"
#include "Image.h"

namespace mcvkp
{

    Image::~Image()
    {
        destroy();
    }

    void Image::destroy()
    {
        if (image != VK_NULL_HANDLE)
        {
            vkDestroyImageView(VulkanGlobal::context.device, imageView, nullptr);
            vkDestroyImage(VulkanGlobal::context.device, image, nullptr);
            vmaFreeMemory(VulkanGlobal::context.allocator, allocation);

            image = VK_NULL_HANDLE;
        }
    }

    VkDescriptorImageInfo Image::getDescriptorInfo(VkImageLayout imageLayout)
    {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = imageLayout;
        imageInfo.imageView = imageView;

        return imageInfo;
    }

    namespace ImageUtils
    {

        VkImageView createImageView(VkImage &image,
                                    VkFormat format,
                                    VkImageAspectFlags aspectFlags,
                                    const uint32_t &mipLevels)
        {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = format;
            viewInfo.subresourceRange.aspectMask = aspectFlags;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = mipLevels;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            VkImageView imageView;
            if (vkCreateImageView(VulkanGlobal::context.device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create texture image view!");
            }

            return imageView;
        }

        void createImage(uint32_t width,
                         uint32_t height,
                         uint32_t mipLevels,
                         VkSampleCountFlagBits numSamples,
                         VkFormat format,
                         VkImageTiling tiling,
                         VkImageUsageFlags usage,
                         VkImageAspectFlags aspectFlags,
                         VmaMemoryUsage memoryUsage,
                         std::shared_ptr<Image> allocatedImage)
        {
            allocatedImage->width = width;
            allocatedImage->height = height;

            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = width;
            imageInfo.extent.height = height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = mipLevels;
            imageInfo.arrayLayers = 1;
            imageInfo.format = format;
            imageInfo.tiling = tiling;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = usage;
            imageInfo.samples = numSamples;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo vmaallocInfo = {};
            vmaallocInfo.usage = memoryUsage;

            if (vmaCreateImage(VulkanGlobal::context.allocator,
                               &imageInfo,
                               &vmaallocInfo,
                               &allocatedImage->image,
                               &allocatedImage->allocation,
                               nullptr) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create buffer");
            }
            allocatedImage->imageView = createImageView(allocatedImage->image,
                                                        format,
                                                        aspectFlags,
                                                        mipLevels);
        }

        void transitionImageLayout(VkImage image,
                                   VkFormat format,
                                   VkImageLayout oldLayout,
                                   VkImageLayout newLayout,
                                   const uint32_t &mipLevels)
        {
            VkCommandBuffer commandBuffer = RenderSystem::beginSingleTimeCommands();
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = oldLayout;
            barrier.newLayout = newLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = mipLevels;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            VkPipelineStageFlags sourceStage;
            VkPipelineStageFlags destinationStage;

            if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            }
            else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL)
            {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            }
            else
            {
                throw std::invalid_argument("unsupported layout transition!");
            }

            vkCmdPipelineBarrier(
                commandBuffer,
                sourceStage, destinationStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            RenderSystem::endSingleTimeCommands(commandBuffer);
        }

        void copyBufferToImage(const VkBuffer &buffer, VkImage image, uint32_t width, uint32_t height)
        {
            VkCommandBuffer commandBuffer = RenderSystem::beginSingleTimeCommands();
            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;

            region.imageOffset = {0, 0, 0};
            region.imageExtent = {
                width,
                height,
                1};

            vkCmdCopyBufferToImage(
                commandBuffer,
                buffer,
                image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region);

            RenderSystem::endSingleTimeCommands(commandBuffer);
        }

        void generateMipmaps(VkImage image,
                             VkFormat imageFormat,
                             int32_t texWidth,
                             int32_t texHeight,
                             const uint32_t &mipLevels)
        {
            // Check if image format supports linear blitting
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(VulkanGlobal::context.physicalDevice, imageFormat, &formatProperties);

            if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
            {
                throw std::runtime_error("texture image format does not support linear blitting!");
            }

            VkCommandBuffer commandBuffer = RenderSystem::beginSingleTimeCommands();

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.image = image;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.subresourceRange.levelCount = 1;

            int32_t mipWidth = texWidth;
            int32_t mipHeight = texHeight;

            for (uint32_t i = 1; i < mipLevels; i++)
            {
                barrier.subresourceRange.baseMipLevel = i - 1;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                vkCmdPipelineBarrier(commandBuffer,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                     0, nullptr,
                                     0, nullptr,
                                     1, &barrier);

                VkImageBlit blit{};
                blit.srcOffsets[0] = {0, 0, 0};
                blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
                blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.srcSubresource.mipLevel = i - 1;
                blit.srcSubresource.baseArrayLayer = 0;
                blit.srcSubresource.layerCount = 1;
                blit.dstOffsets[0] = {0, 0, 0};
                blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
                blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.dstSubresource.mipLevel = i;
                blit.dstSubresource.baseArrayLayer = 0;
                blit.dstSubresource.layerCount = 1;

                vkCmdBlitImage(commandBuffer,
                               image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1, &blit,
                               VK_FILTER_LINEAR);

                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                vkCmdPipelineBarrier(commandBuffer,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                                     0, nullptr,
                                     0, nullptr,
                                     1, &barrier);

                if (mipWidth > 1)
                    mipWidth /= 2;
                if (mipHeight > 1)
                    mipHeight /= 2;
            }

            barrier.subresourceRange.baseMipLevel = mipLevels - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            RenderSystem::endSingleTimeCommands(commandBuffer);
        }

        void createTextureImage(const std::string &path,
                                std::shared_ptr<Image> allocatedImage,
                                uint32_t &mipLevels)
        {
            int texWidth, texHeight, texChannels;
            // std::string path = path_prefix + "/textures/viking_room.png";
            StbImageImpl tex(path, texWidth, texHeight, texChannels);

            mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

            VkDeviceSize imageSize = texWidth * texHeight * 4;

            if (!tex.pixels)
            {
                throw std::runtime_error("failed to load texture image!");
            }

            mcvkp::Buffer stagingBuffer;
            BufferUtils::create(&stagingBuffer, tex.pixels, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

            createImage(texWidth,
                        texHeight,
                        mipLevels,
                        VK_SAMPLE_COUNT_1_BIT,
                        VK_FORMAT_R8G8B8A8_SRGB,
                        VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_IMAGE_ASPECT_COLOR_BIT,
                        VMA_MEMORY_USAGE_GPU_ONLY,
                        allocatedImage);
            std::cout << "creating texture" << std::endl;
            transitionImageLayout(allocatedImage->image,
                                  VK_FORMAT_R8G8B8A8_SRGB,
                                  VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  mipLevels);
            copyBufferToImage(stagingBuffer.buffer, allocatedImage->image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
            generateMipmaps(allocatedImage->image, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);
            // stagingBuffer.destroy();
        }

        void createTextureSampler(std::shared_ptr<VkSampler> textureSampler, uint32_t &mipLevels)
        {
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.anisotropyEnable = VK_FALSE;
            samplerInfo.maxAnisotropy = 1.0f;
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.minLod = 0.0f; // Optional
            samplerInfo.maxLod = static_cast<float>(mipLevels);
            samplerInfo.mipLodBias = 0.0f; // Optional

            if (vkCreateSampler(VulkanGlobal::context.device, &samplerInfo, nullptr, textureSampler.get()) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create texture sampler!");
            }
        }

    }

    Texture::Texture(const std::string &path)
    {
        m_image = std::make_shared<Image>();
        m_sampler = std::make_shared<VkSampler>();

        ImageUtils::createTextureImage(path, m_image, m_mips);
        ImageUtils::createTextureSampler(m_sampler, m_mips);
    }

    Texture::Texture(const std::shared_ptr<Image> &image) : m_image(image)
    {
        m_sampler = std::make_shared<VkSampler>();
        ImageUtils::createTextureSampler(m_sampler, m_mips);
    }

    Texture::~Texture()
    {
        vkDestroySampler(VulkanGlobal::context.device, *m_sampler, nullptr);
    }

    VkDescriptorImageInfo Texture::getDescriptorInfo()
    {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_image->imageView;
        imageInfo.sampler = *m_sampler;

        return imageInfo;
    }

}

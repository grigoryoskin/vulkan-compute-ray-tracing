#pragma once

#include "../utils/vulkan.h"
#include <vector>
#include "VulkanApplicationContext.h"
#include "../memory/Image.h"

class VulkanSwapchain
{
public:
    VulkanSwapchain();
    ~VulkanSwapchain();

    const VkSwapchainKHR &getBody() const;
    const VkExtent2D &getExtent() const;
    const VkFormat &getFormat() const;
    const std::vector<VkImage> &getImages() const;
    const std::vector<VkImageView> &getImageViews() const;

private:
    void createSwapChain();

private:
    vkb::Swapchain m_vkbSwapchain;
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_imageViews;
};

namespace VulkanGlobal
{
    extern const VulkanSwapchain swapchainContext;
}

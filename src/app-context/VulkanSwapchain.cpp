#include "./VulkanSwapchain.h"

#include <vector>
#include <iostream>

VulkanSwapchain::VulkanSwapchain()
{
    createSwapChain();
}

VulkanSwapchain::~VulkanSwapchain()
{
    std::cout << "Destroying swapchain"
              << "\n";
    for (size_t i = 0; i < m_imageViews.size(); i++)
    {
        vkDestroyImageView(VulkanGlobal::context.getDevice(), m_imageViews[i], nullptr);
    }

    vkb::destroy_swapchain(m_vkbSwapchain);
}

void VulkanSwapchain::createSwapChain()
{
    vkb::SwapchainBuilder swapchain_builder{VulkanGlobal::context.getVkbDevice()};
    auto swap_ret = swapchain_builder.set_old_swapchain(m_vkbSwapchain)
                        .build();
    if (!swap_ret)
    {
        // If it failed to create a swapchain, the old swapchain handle is invalid.
        throw std::runtime_error("Failed to create swapchain. Error: " + swap_ret.error().message());
        m_vkbSwapchain.swapchain = VK_NULL_HANDLE;
    }
    // Even though we recycled the previous swapchain, we need to free its resources.
    vkb::destroy_swapchain(m_vkbSwapchain);
    // Get the new swapchain and place it in our variable
    m_vkbSwapchain = swap_ret.value();

    uint32_t imageCount = m_vkbSwapchain.image_count;

    auto image_ret = m_vkbSwapchain.get_images();
    if(!image_ret) {

    }
    m_images = image_ret.value();

    auto image_view_ret = m_vkbSwapchain.get_image_views();
    if(!image_view_ret) {

    }
    m_imageViews = image_view_ret.value();
}

const VkSwapchainKHR &VulkanSwapchain::getBody() const
{
    return m_vkbSwapchain.swapchain;
}

const VkExtent2D &VulkanSwapchain::getExtent() const
{
    return m_vkbSwapchain.extent;
}

const VkFormat &VulkanSwapchain::getFormat() const
{
    return m_vkbSwapchain.image_format;
}

const std::vector<VkImage> &VulkanSwapchain::getImages() const
{
    return m_images;
}

const std::vector<VkImageView> &VulkanSwapchain::getImageViews() const
{
    return m_imageViews;
}

#include "./VulkanSwapchain.h"

#include <vector>
#include <iostream>

VulkanSwapchain::VulkanSwapchain() {
    createSwapChain();
    createImageViews();
}

VulkanSwapchain::~VulkanSwapchain() {
    std::cout << "Destroying swapchain" << "\n";
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        vkDestroyImageView(VulkanGlobal::context.device, swapChainImageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(VulkanGlobal::context.device, swapChain, nullptr);
}

VkSurfaceFormatKHR VulkanSwapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR VulkanSwapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(VulkanGlobal::context.window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::max(capabilities.minImageExtent.width,
                                    std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height,
                                    std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

void VulkanSwapchain::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = VulkanGlobal::context.querySwapChainSupport();

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    // Precalculated this to make it globally available.
    uint32_t imageCount = VulkanGlobal::context.swapChainImageCount;

    std::cout << "Swap chain image count: " << imageCount << "\n";

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = VulkanGlobal::context.surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = VulkanGlobal::context.queueFamilyIndices;
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(),
                                        indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(VulkanGlobal::context.device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(VulkanGlobal::context.device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(VulkanGlobal::context.device, swapChain, &imageCount, swapChainImages.data());
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void VulkanSwapchain::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());
    uint32_t mipLevels = 1;
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = 
            mcvkp::ImageUtils::createImageView(swapChainImages[i],
                                            swapChainImageFormat,
                                            VK_IMAGE_ASPECT_COLOR_BIT,
                                            mipLevels);
    }
}
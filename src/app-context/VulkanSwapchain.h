#pragma once

#include "../utils/vulkan.h"
#include <vector>
#include "VulkanApplicationContext.h"
#include "../memory/Image.h"

class VulkanSwapchain {
    public:
        VkSwapchainKHR swapChain;
        std::vector<VkImage> swapChainImages;
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
        std::vector<VkImageView> swapChainImageViews;
        
        VulkanSwapchain();
        ~VulkanSwapchain();

    private:
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

        void createSwapChain();

        void createImageViews();
};

namespace VulkanGlobal {
    extern const VulkanSwapchain swapchainContext;
}
#pragma once

#include "../utils/vulkan.h"
#include "vk_mem_alloc.h"
#include <optional>
#include <string>
#include <vector>
#include <set>

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_KHR_portability_subset"
};
#ifdef NDEBUG
    const bool enableValidationLayers = true;
#else
    const bool enableValidationLayers = true;
#endif

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VulkanApplicationContext {
    public:
        GLFWwindow* window;
        VkInstance instance;
        VkPhysicalDevice physicalDevice;
        // Logical device.
        VkDevice device;
        VkSurfaceKHR surface;
        QueueFamilyIndices queueFamilyIndices;
        VkQueue graphicsQueue;
        VkQueue presentQueue;
        VkCommandPool commandPool;
        VmaAllocator allocator;
        VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

        uint32_t swapChainImageCount;
        
        VulkanApplicationContext() ;

        ~VulkanApplicationContext() ;

        SwapChainSupportDetails querySwapChainSupport() const;

        VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,
                                     VkImageTiling tiling, 
                                     VkFormatFeatureFlags features) const;
    private:

        void initWindow() ;

        void createSurface() ;

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;

        bool checkValidationLayerSupport();

        void createInstance();

        VkSampleCountFlagBits getMaxUsableSampleCount();

        bool checkDeviceExtensionSupport(VkPhysicalDevice device);

        bool isDeviceSuitable(VkPhysicalDevice device, QueueFamilyIndices indices);

        void pickPhysicalDevice();

        void createLogicalDevice();

        void createAllocator();

        void createCommandPool();

        void initSwapchainImageCount();

};

namespace VulkanGlobal {
    extern const VulkanApplicationContext context;
}

#pragma once

#include "../utils/vulkan.h"
#include "vk_mem_alloc.h"
#include <unordered_map>
#include <optional>
#include <string>
#include <vector>
#include <set>

const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_KHR_portability_subset"};
#ifdef NDEBUG
const bool enableValidationLayers = true;
#else
const bool enableValidationLayers = true;
#endif

const uint32_t WIDTH = 600;
const uint32_t HEIGHT = 600;

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> computeFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value() && computeFamily.has_value();
    }
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VulkanApplicationContext
{
public:
    GLFWwindow *window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    // Logical device.
    VkDevice device;
    VkSurfaceKHR surface;
    QueueFamilyIndices queueFamilyIndices;
    VkQueue graphicsQueue;
    VkQueue computeQueue;
    VkQueue presentQueue;
    VmaAllocator allocator;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    uint32_t swapChainImageCount;
    VkCommandPool graphicsCommandPool;
    VkCommandPool computeCommandPool;
    VulkanApplicationContext();

    ~VulkanApplicationContext();

    SwapChainSupportDetails querySwapChainSupport() const;

    VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates,
                                 VkImageTiling tiling,
                                 VkFormatFeatureFlags features) const;

private:
    void initWindow();

    void createSurface();

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

    void createCommandPool(uint32_t queueFamilyIndex, VkCommandPool &commandPool);

    void initSwapchainImageCount();
};

namespace VulkanGlobal
{
    extern const VulkanApplicationContext context;
}
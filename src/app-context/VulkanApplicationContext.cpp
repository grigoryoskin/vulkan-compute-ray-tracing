#include "../utils/vulkan.h"
#include <iostream>
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include "VulkanApplicationContext.h"

VulkanApplicationContext::VulkanApplicationContext()
{
    initWindow();
    createInstance();
    createSurface();
    createDevice();

    createCommandPool();
}

VulkanApplicationContext::~VulkanApplicationContext()
{
    std::cout << "Destroying context"
              << "\n";
    vkDestroyCommandPool(m_vkbDevice.device, m_commandPool, nullptr);
    vmaDestroyAllocator(m_allocator);
    vkDestroySurfaceKHR(m_vkbInstance.instance, m_surface, nullptr);

    vkb::destroy_device(m_vkbDevice);
    vkb::destroy_instance(m_vkbInstance);
    glfwDestroyWindow(m_window);
}

void VulkanApplicationContext::initWindow()
{
    glfwInit();
    // This tells glfw not to use opengl.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

void VulkanApplicationContext::createSurface()
{
    if (glfwCreateWindowSurface(m_vkbInstance.instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create window surface!");
    }
}

void VulkanApplicationContext::createInstance()
{
    vkb::InstanceBuilder instance_builder;
    auto instance_builder_return = instance_builder
                                       // Instance creation configuration
                                       .request_validation_layers()
                                       .use_default_debug_messenger()
                                       .enable_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)
                                       .build();
    if (!instance_builder_return)
    {
        throw std::runtime_error("Failed to create Vulkan instance. Error: " + instance_builder_return.error().message());
    }
    m_vkbInstance = instance_builder_return.value();
}

void VulkanApplicationContext::createDevice()
{
    vkb::PhysicalDeviceSelector phys_device_selector(m_vkbInstance);
    auto phys_dev_ret = phys_device_selector
                            .add_desired_extension("VK_KHR_portability_subset")
                            .set_surface(m_surface)
                            .select();
    if (!phys_dev_ret)
    {
        throw std::runtime_error("Failed to create physical device. Error: " + phys_dev_ret.error().message());
    }
    //m_physicalDevice = phys_dev_ret.value();
    vkb::DeviceBuilder device_builder{phys_dev_ret.value()};
    auto dev_ret = device_builder.build();
    if (!dev_ret)
    {
        throw std::runtime_error("Failed to create device. Error: " + dev_ret.error().message());
    }
    m_vkbDevice = dev_ret.value();

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_0;
    allocatorInfo.physicalDevice = phys_dev_ret.value();
    allocatorInfo.device = m_vkbDevice.device;
    allocatorInfo.instance = m_vkbInstance.instance;

    if (vmaCreateAllocator(&allocatorInfo, &m_allocator) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create command memory allocator!");
    }
}

void VulkanApplicationContext::createCommandPool()
{
    auto g_queue_ret = m_vkbDevice.get_queue(vkb::QueueType::graphics);
    if (!g_queue_ret)
    {
        throw std::runtime_error("Failed to create graphics queue. Error: " + g_queue_ret.error().message());
    }
    m_graphicsQueue = g_queue_ret.value();

    auto p_queue_ret = m_vkbDevice.get_queue(vkb::QueueType::present);
    if (!p_queue_ret)
    {
        throw std::runtime_error("Failed to create present queue. Error: " + p_queue_ret.error().message());
    }
    m_presentQueue = p_queue_ret.value();

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = m_vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
    poolInfo.flags = 0; // Optional
    if (vkCreateCommandPool(m_vkbDevice.device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create command pool!");
    }
}

VkFormat VulkanApplicationContext::findSupportedFormat(const std::vector<VkFormat> &candidates,
                                                       VkImageTiling tiling,
                                                       VkFormatFeatureFlags features) const
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_vkbDevice.physical_device, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

const VkDevice &VulkanApplicationContext::getDevice() const
{
    return m_vkbDevice.device;
}

const VkPhysicalDevice &VulkanApplicationContext::getPhysicalDevice() const
{
    return m_vkbDevice.physical_device;
}

const VkQueue &VulkanApplicationContext::getGraphicsQueue() const
{
    return m_graphicsQueue;
}

const VkQueue &VulkanApplicationContext::getPresentQueue() const
{
    return m_presentQueue;
}

const VkCommandPool &VulkanApplicationContext::getCommandPool() const
{
    return m_commandPool;
}

const VmaAllocator &VulkanApplicationContext::getAllocator() const
{
    return m_allocator;
}

const vkb::Device &VulkanApplicationContext::getVkbDevice() const
{
    return m_vkbDevice;
}

GLFWwindow *VulkanApplicationContext::getWindow() const
{
    return m_window;
}

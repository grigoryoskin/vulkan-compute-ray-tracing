#pragma once

#include "../utils/vulkan.h"
#include "vk_mem_alloc.h"
#include <optional>
#include <string>
#include <vector>
#include <set>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class VulkanApplicationContext {
    public:        
        VulkanApplicationContext() ;

        ~VulkanApplicationContext() ;
        
        VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,
                                VkImageTiling tiling, 
                                VkFormatFeatureFlags features) const;
        
        const VkDevice& getDevice() const; 

        const VkPhysicalDevice& getPhysicalDevice() const; 

        const VkQueue& getGraphicsQueue() const;

        const VkQueue& getPresentQueue() const;

        const VkCommandPool& getCommandPool() const;

        const VmaAllocator& getAllocator() const;

        const vkb::Device& getVkbDevice() const;

        GLFWwindow* getWindow() const;

    private:
        void initWindow() ;

        void createSurface() ;

        void createInstance();

        void createDevice();

        void createCommandPool();

    private:
        GLFWwindow* m_window;
        vkb::Instance m_vkbInstance;
        VkSurfaceKHR m_surface;
        VkQueue m_graphicsQueue;
        VkQueue m_presentQueue;
        VkCommandPool m_commandPool;
        VmaAllocator m_allocator;
        vkb::Device m_vkbDevice;
};

namespace VulkanGlobal {
    extern const VulkanApplicationContext context;
}

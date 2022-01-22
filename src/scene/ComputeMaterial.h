#pragma once
#include <vector>
#include <memory>
#include "../memory/Buffer.h"
#include "../utils/vulkan.h"
#include "../memory/Image.h"
#include "../app-context/VulkanSwapchain.h"
#include "./Material.h"

namespace mcvkp
{
    class ComputeMaterial : public Material
    {
    public:
        ComputeMaterial(const std::string &computeShaderPath);

        void init();

        void bind(VkCommandBuffer &commandBuffer, size_t currentFrame);

    private:
        void __initComputePipeline(const std::string &computeShaderPath);

    private:
        std::string m_computeShaderPath;
    };
}

#pragma once
#include <vector>
#include <memory>
#include "../memory/Buffer.h"
#include "../utils/vulkan.h"
#include "../memory/Image.h"
#include "../app-context/VulkanSwapchain.h"

namespace mcvkp
{
    template <typename T>
    struct Descriptor
    {
        std::shared_ptr<T> data;
        VkShaderStageFlags shaderStageFlags;
    };

    class Material
    {
    public:
        Material(
            const std::string &vertexShaderPath,
            const std::string &fragmentShaderPath);

        Material();

        ~Material();

        void addTexture(const std::shared_ptr<Texture> &texture, VkShaderStageFlags shaderStageFlags);

        void addStorageImage(const std::shared_ptr<Image> &image, VkShaderStageFlags shaderStageFlags);

        void addUniformBufferBundle(const std::shared_ptr<BufferBundle> &bufferBundle, VkShaderStageFlags shaderStageFlags);

        void addStorageBufferBundle(const std::shared_ptr<BufferBundle> &bufferBundle, VkShaderStageFlags shaderStageFlags);

        const std::vector<Descriptor<BufferBundle> > &getUniformBufferBundles() const;

        const std::vector<Descriptor<BufferBundle> > &getStorageBufferBundles() const;

        const std::vector<Descriptor<Texture> > &getTextures() const;

        const std::vector<Descriptor<Image> > &getStorageImages() const;

        // Initialize material when adding to a scene.
        void init(const VkRenderPass &renderPass);

        void bind(VkCommandBuffer &commandBuffer, size_t currentFrame);

    protected:
        void __initDescriptorSetLayout();
        void __initDescriptorPool();
        void __initDescriptorSets();
        void __initPipeline(
            const VkExtent2D &swapChainExtent,
            const VkRenderPass &renderPass,
            std::string vertexShaderPath,
            std::string fragmentShaderPath);
        VkShaderModule __createShaderModule(const std::vector<char> &code);

    protected:
        std::vector<Descriptor<BufferBundle> > m_uniformBufferBundleDescriptors;
        std::vector<Descriptor<BufferBundle> > m_storageBufferBundleDescriptors;
        std::vector<Descriptor<Texture> > m_textureDescriptors;
        std::vector<Descriptor<Image> > m_storageImageDescriptors;

        std::string m_vertexShaderPath;
        std::string m_fragmentShaderPath;

        bool m_initialized;

        uint32_t m_descriptorSetsSize;

        VkPipelineLayout m_pipelineLayout;
        VkPipeline m_pipeline;

        VkDescriptorPool m_descriptorPool;
        std::vector<VkDescriptorSet> m_descriptorSets;
        VkDescriptorSetLayout m_descriptorSetLayout;
    };
}

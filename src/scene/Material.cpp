#include <vector>
#include <memory>
#include "../utils/readfile.h"

#include "Material.h"

namespace mcvkp
{
    Material::Material(
        const std::string &vertexShaderPath,
        const std::string &fragmentShaderPath) : m_fragmentShaderPath(fragmentShaderPath), m_vertexShaderPath(vertexShaderPath), m_initialized(false)
    {
        m_descriptorSetsSize = VulkanGlobal::swapchainContext.getImages().size();
    }

    Material::Material()
    {
        m_descriptorSetsSize = VulkanGlobal::swapchainContext.getImages().size();
    }

    Material::~Material()
    {
        std::cout << "Destroying material"
                  << "\n";
        vkDestroyDescriptorSetLayout(VulkanGlobal::context.getDevice(), m_descriptorSetLayout, nullptr);
        vkDestroyPipeline(VulkanGlobal::context.getDevice(), m_pipeline, nullptr);
        vkDestroyPipelineLayout(VulkanGlobal::context.getDevice(), m_pipelineLayout, nullptr);
        vkDestroyDescriptorPool(VulkanGlobal::context.getDevice(), m_descriptorPool, nullptr);
    }

    VkShaderModule Material::__createShaderModule(const std::vector<char> &code)
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(VulkanGlobal::context.getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create shader module!");
        }
        return shaderModule;
    }

    void Material::addTexture(const std::shared_ptr<Texture> &texture, VkShaderStageFlags shaderStageFlags)
    {
        m_textureDescriptors.push_back({texture, shaderStageFlags});
    }

    void Material::addUniformBufferBundle(const std::shared_ptr<BufferBundle> &bufferBundle, VkShaderStageFlags shaderStageFlags)
    {
        m_uniformBufferBundleDescriptors.push_back({bufferBundle, shaderStageFlags});
    }

    void Material::addStorageBufferBundle(const std::shared_ptr<BufferBundle> &bufferBundle, VkShaderStageFlags shaderStageFlags)
    {
        m_storageBufferBundleDescriptors.push_back({bufferBundle, shaderStageFlags});
    }

    void Material::addStorageImage(const std::shared_ptr<Image> &image, VkShaderStageFlags shaderStageFlags)
    {
        m_storageImageDescriptors.push_back({image, shaderStageFlags});
    }

    const std::vector<Descriptor<BufferBundle> > &Material::getUniformBufferBundles() const
    {
        return m_uniformBufferBundleDescriptors;
    }

    const std::vector<Descriptor<BufferBundle> > &Material::getStorageBufferBundles() const
    {
        return m_storageBufferBundleDescriptors;
    }

    const std::vector<Descriptor<Texture> > &Material::getTextures() const
    {
        return m_textureDescriptors;
    }

    const std::vector<Descriptor<Image> > &Material::getStorageImages() const
    {
        return m_storageImageDescriptors;
    }

    // Initialize material when adding to a scene.
    void Material::init(const VkRenderPass &renderPass)
    {
        if (m_initialized)
        {
            return;
        }
        __initDescriptorSetLayout();
        __initPipeline(VulkanGlobal::swapchainContext.getExtent(), renderPass, m_vertexShaderPath, m_fragmentShaderPath);
        __initDescriptorPool();
        __initDescriptorSets();
        m_initialized = true;
    }

    void Material::__initPipeline(const VkExtent2D &swapChainExtent,
                                  const VkRenderPass &renderPass,
                                  std::string vertexShaderPath,
                                  std::string fragmentShaderPath)
    {
        auto vertShaderCode = readFile(vertexShaderPath);
        auto fragShaderCode = readFile(fragmentShaderPath);
        VkShaderModule vertShaderModule = __createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = __createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional
        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapChainExtent.width;
        viewport.height = (float)swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f;          // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f;    // Optional

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.sampleShadingEnable = VK_TRUE;    // enable sample shading in the pipeline
        multisampling.minSampleShading = .2f;           // min fraction for sample shading; closer to one is smoother
        multisampling.pSampleMask = nullptr;            // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE;      // Optional

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_LINE_WIDTH};

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;

        if (vkCreatePipelineLayout(VulkanGlobal::context.getDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {};  // Optional

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr; // Optional
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = nullptr; // Optional
        pipelineInfo.layout = m_pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1;              // Optional
        pipelineInfo.pDepthStencilState = &depthStencil;

        if (vkCreateGraphicsPipelines(VulkanGlobal::context.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(VulkanGlobal::context.getDevice(), fragShaderModule, nullptr);
        vkDestroyShaderModule(VulkanGlobal::context.getDevice(), vertShaderModule, nullptr);
    }

    void Material::__initDescriptorSetLayout()
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        for (size_t buffer_i = 0; buffer_i < m_uniformBufferBundleDescriptors.size(); buffer_i++)
        {
            VkDescriptorSetLayoutBinding uboLayoutBinding{};
            uboLayoutBinding.binding = buffer_i;
            uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uboLayoutBinding.descriptorCount = 1;
            uboLayoutBinding.stageFlags = m_uniformBufferBundleDescriptors[buffer_i].shaderStageFlags;
            uboLayoutBinding.pImmutableSamplers = nullptr; // Optional
            bindings.push_back(uboLayoutBinding);
        }

        for (size_t tex_i = 0; tex_i < m_textureDescriptors.size(); tex_i++)
        {
            VkDescriptorImageInfo imageInfo = m_textureDescriptors[tex_i].data->getDescriptorInfo();

            size_t binding = m_uniformBufferBundleDescriptors.size() + tex_i;
            VkDescriptorSetLayoutBinding samplerLayoutBinding{};
            samplerLayoutBinding.binding = binding;
            samplerLayoutBinding.descriptorCount = 1;
            samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            samplerLayoutBinding.pImmutableSamplers = nullptr;
            samplerLayoutBinding.stageFlags = m_textureDescriptors[tex_i].shaderStageFlags;
            bindings.push_back(samplerLayoutBinding);
        }

        for (size_t tex_i = 0; tex_i < m_storageImageDescriptors.size(); tex_i++)
        {
            VkDescriptorImageInfo imageInfo = m_storageImageDescriptors[tex_i].data->getDescriptorInfo(VK_IMAGE_LAYOUT_GENERAL);

            size_t binding = m_uniformBufferBundleDescriptors.size() + m_textureDescriptors.size() + tex_i;
            VkDescriptorSetLayoutBinding samplerLayoutBinding{};
            samplerLayoutBinding.binding = binding;
            samplerLayoutBinding.descriptorCount = 1;
            samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            samplerLayoutBinding.pImmutableSamplers = nullptr;
            samplerLayoutBinding.stageFlags = m_storageImageDescriptors[tex_i].shaderStageFlags;
            bindings.push_back(samplerLayoutBinding);
        }

        for (size_t buffer_i = 0; buffer_i < m_storageBufferBundleDescriptors.size(); buffer_i++)
        {
            size_t binding = m_uniformBufferBundleDescriptors.size() + m_textureDescriptors.size() + m_storageImageDescriptors.size() + buffer_i;
            VkDescriptorSetLayoutBinding storageBufferBinding{};
            storageBufferBinding.binding = binding;
            storageBufferBinding.descriptorCount = 1;
            storageBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            storageBufferBinding.pImmutableSamplers = nullptr;
            storageBufferBinding.stageFlags = m_storageBufferBundleDescriptors[buffer_i].shaderStageFlags;
            bindings.push_back(storageBufferBinding);
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(VulkanGlobal::context.getDevice(), &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    void Material::__initDescriptorPool()
    {
        std::vector<VkDescriptorPoolSize> poolSizes{};

        for (size_t buffer_i = 0; buffer_i < m_uniformBufferBundleDescriptors.size(); buffer_i++)
        {
            VkDescriptorPoolSize size;
            size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            size.descriptorCount = static_cast<uint32_t>(m_descriptorSetsSize);
            poolSizes.push_back(size);
        }

        for (size_t tex_i = 0; tex_i < m_textureDescriptors.size(); tex_i++)
        {
            VkDescriptorPoolSize size;
            size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            size.descriptorCount = static_cast<uint32_t>(m_descriptorSetsSize);
            poolSizes.push_back(size);
        }

        for (size_t tex_i = 0; tex_i < m_storageImageDescriptors.size(); tex_i++)
        {
            VkDescriptorPoolSize size;
            size.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            size.descriptorCount = static_cast<uint32_t>(m_descriptorSetsSize);
            poolSizes.push_back(size);
        }

        for (size_t buffer_i = 0; buffer_i < m_storageBufferBundleDescriptors.size(); buffer_i++)
        {
            VkDescriptorPoolSize size;
            size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            size.descriptorCount = static_cast<uint32_t>(m_descriptorSetsSize);
            poolSizes.push_back(size);
        }

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(m_descriptorSetsSize);

        if (vkCreateDescriptorPool(VulkanGlobal::context.getDevice(), &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void Material::__initDescriptorSets()
    {
        std::vector<VkDescriptorSetLayout> layouts(m_descriptorSetsSize, m_descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(m_descriptorSetsSize);
        allocInfo.pSetLayouts = layouts.data();

        m_descriptorSets.resize(m_descriptorSetsSize);
        if (vkAllocateDescriptorSets(VulkanGlobal::context.getDevice(), &allocInfo, m_descriptorSets.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        size_t numDescriptors = m_uniformBufferBundleDescriptors.size() + m_textureDescriptors.size() + m_storageImageDescriptors.size();

        for (size_t i = 0; i < m_descriptorSetsSize; i++)
        {
            // VkDescriptorBufferInfo bufferInfo = uniformBuffers[i].getDescriptorInfo();
            // VkDescriptorImageInfo imageInfo = textureImage.getDescriptorInfo();
            // VkDescriptorBufferInfo sharedBufferInfo = (*sharedUniformBuffers)[i].getDescriptorInfo();

            std::vector<VkWriteDescriptorSet> descriptorWrites;
            descriptorWrites.reserve(numDescriptors);
            std::vector<VkDescriptorBufferInfo> bufferDescInfos;
            for (size_t buffer_i = 0; buffer_i < m_uniformBufferBundleDescriptors.size(); buffer_i++)
            {
                bufferDescInfos.push_back(m_uniformBufferBundleDescriptors[buffer_i].data->buffers[i]->getDescriptorInfo());
            }

            for (size_t buffer_i = 0; buffer_i < m_uniformBufferBundleDescriptors.size(); buffer_i++)
            {
                VkWriteDescriptorSet descriptorSet{};
                descriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorSet.dstSet = m_descriptorSets[i];
                descriptorSet.dstBinding = buffer_i;
                descriptorSet.dstArrayElement = 0;
                descriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorSet.descriptorCount = 1;
                descriptorSet.pBufferInfo = &bufferDescInfos[buffer_i];

                descriptorWrites.push_back(descriptorSet);
            }
            std::vector<VkDescriptorImageInfo> imageInfos;
            for (size_t tex_i = 0; tex_i < m_textureDescriptors.size(); tex_i++)
            {
                imageInfos.push_back(m_textureDescriptors[tex_i].data->getDescriptorInfo());
            }

            for (size_t tex_i = 0; tex_i < m_textureDescriptors.size(); tex_i++)
            {
                size_t binding = m_uniformBufferBundleDescriptors.size() + tex_i;
                VkWriteDescriptorSet descriptorSet{};
                descriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorSet.dstSet = m_descriptorSets[i];
                descriptorSet.dstBinding = binding;
                descriptorSet.dstArrayElement = 0;
                descriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorSet.descriptorCount = 1;
                descriptorSet.pImageInfo = &imageInfos[tex_i];

                descriptorWrites.push_back(descriptorSet);
            }

            std::vector<VkDescriptorImageInfo> storageImageInfos;
            for (size_t tex_i = 0; tex_i < m_storageImageDescriptors.size(); tex_i++)
            {
                storageImageInfos.push_back(m_storageImageDescriptors[tex_i].data->getDescriptorInfo(VK_IMAGE_LAYOUT_GENERAL));
            }

            for (size_t tex_i = 0; tex_i < m_storageImageDescriptors.size(); tex_i++)
            {
                size_t binding = m_uniformBufferBundleDescriptors.size() + m_textureDescriptors.size() + tex_i;
                VkWriteDescriptorSet descriptorSet{};
                descriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorSet.dstSet = m_descriptorSets[i];
                descriptorSet.dstBinding = binding;
                descriptorSet.dstArrayElement = 0;
                descriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                descriptorSet.descriptorCount = 1;
                descriptorSet.pImageInfo = &storageImageInfos[tex_i];

                descriptorWrites.push_back(descriptorSet);
            }

            std::vector<VkDescriptorBufferInfo> storageBufferInfos;
            for (size_t buffer_i = 0; buffer_i < m_storageBufferBundleDescriptors.size(); buffer_i++)
            {
                storageBufferInfos.push_back(m_storageBufferBundleDescriptors[buffer_i].data->buffers[i]->getDescriptorInfo());
            }

            for (size_t buffer_i = 0; buffer_i < m_storageBufferBundleDescriptors.size(); buffer_i++)
            {
                size_t binding = m_uniformBufferBundleDescriptors.size() + m_textureDescriptors.size() + m_storageImageDescriptors.size() + buffer_i;
                VkWriteDescriptorSet descriptorSet{};
                descriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorSet.dstSet = m_descriptorSets[i];
                descriptorSet.dstBinding = binding;
                descriptorSet.dstArrayElement = 0;
                descriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                descriptorSet.descriptorCount = 1;
                descriptorSet.pBufferInfo = &storageBufferInfos[buffer_i];

                descriptorWrites.push_back(descriptorSet);
            }
            vkUpdateDescriptorSets(VulkanGlobal::context.getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }

    void Material::bind(VkCommandBuffer &commandBuffer, size_t currentFrame)
    {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[currentFrame], 0, nullptr);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    }
}

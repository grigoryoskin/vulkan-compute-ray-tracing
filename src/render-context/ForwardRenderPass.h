#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include "../utils/vulkan.h"
#include "../memory/Image.h"
#include "RenderPass.h"

namespace mcvkp
{
    class ForwardRenderPass : public RenderPass
    {
    public:
        std::shared_ptr<VkRenderPass> getBody() override;

        std::shared_ptr<VkFramebuffer> getFramebuffer(size_t index) override;

        ForwardRenderPass();

        ~ForwardRenderPass();

        std::shared_ptr<mcvkp::Image> getColorImage() override ;
        std::shared_ptr<mcvkp::Image> getDepthImage();

    private:
        std::shared_ptr<mcvkp::Image> m_colorImage;
        std::shared_ptr<mcvkp::Image> m_depthImage;
        std::shared_ptr<VkRenderPass> m_renderPass;
        std::shared_ptr<VkFramebuffer> m_framebuffer;

        void createRenderPass();
       
        void createFramebuffers();

        VkFormat findDepthFormat();

        bool hasStencilComponent(VkFormat format);

        void createDepthResources();

        void createColorResources();
    };
}
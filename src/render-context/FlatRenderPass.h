#pragma once

#include <iostream>
#include <vector>
#include <array>
#include "../utils/vulkan.h"
#include "../memory/Image.h"
#include "RenderPass.h"

namespace mcvkp
{
class FlatRenderPass : public RenderPass{
    public:
        std::shared_ptr<VkRenderPass> getBody() override;

        std::shared_ptr<VkFramebuffer> getFramebuffer(size_t index) override;

        // This shouldn't be called. Sorry for sloppy OOP.
        std::shared_ptr<mcvkp::Image> getColorImage() override;

        FlatRenderPass();

        ~FlatRenderPass();

    private:
        std::shared_ptr<VkRenderPass> m_renderPass;

        // Since this render pass is presented to the screen we need to be able to render to one framebuffer
        // while presenting the other.
        std::vector<std::shared_ptr<VkFramebuffer> > m_swapChainFramebuffers;

        void createRenderPass();

        void createFramebuffers();
};
}
#pragma once

#include "../utils/vulkan.h"
#include "../memory/Image.h"
#include <memory>

namespace mcvkp {
class RenderPass {
    public:
        virtual std::shared_ptr<VkRenderPass> getBody() = 0;
        virtual std::shared_ptr<VkFramebuffer> getFramebuffer(size_t index) = 0;
        virtual std::shared_ptr<mcvkp::Image> getColorImage() = 0;
};
}
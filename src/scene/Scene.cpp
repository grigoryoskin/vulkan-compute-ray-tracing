#include "Scene.h"

namespace mcvkp
{
    Scene::Scene(RenderPassType RenderPassType)
    {
        switch (RenderPassType)
        {
        case RenderPassType::eFlat:
            _initFlatRenderPass();
            break;
        case RenderPassType::eForward:
            _initForwardRenderPass();
            break;
        default:
            break;
        }
    }

    void Scene::_initForwardRenderPass()
    {
        m_RenderPass = std::make_shared<ForwardRenderPass>();
    }

    void Scene::_initFlatRenderPass()
    {
        m_RenderPass = std::make_shared<FlatRenderPass>();
    }

    void Scene::addModel(std::shared_ptr<DrawableModel> model)
    {
        model->getMaterial()->init(*m_RenderPass->getBody());
        m_models.push_back(model);
    }

    std::shared_ptr<RenderPass> Scene::getRenderPass()
    {
        return m_RenderPass;
    }

    void Scene::writeRenderCommand(VkCommandBuffer &commandBuffer, const size_t currentFrame)
    {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = *m_RenderPass->getBody();
        renderPassInfo.framebuffer = *m_RenderPass->getFramebuffer(currentFrame);
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = VulkanGlobal::swapchainContext.swapChainExtent;
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {1.0f, 0.5f, 1.0f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        for (std::shared_ptr<DrawableModel> model : m_models)
        {
            model->drawCommand(commandBuffer, currentFrame);
        }

        vkCmdEndRenderPass(commandBuffer);
    }
}

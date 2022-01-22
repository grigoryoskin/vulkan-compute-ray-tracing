#include <iostream>
#include <vector>
#include <array>
#include "../utils/vulkan.h"
#include "FlatRenderPass.h"
#include "../app-context/VulkanApplicationContext.h"
#include "../app-context/VulkanSwapchain.h"

namespace mcvkp
{
    std::shared_ptr<VkRenderPass> FlatRenderPass::getBody() 
    {
        return m_renderPass;
    }

    std::shared_ptr<VkFramebuffer> FlatRenderPass::getFramebuffer(size_t index) 
    {
        return m_swapChainFramebuffers[index];
    }

    // This shouldn't be called. Sorry for sloppy OOP.
    std::shared_ptr<mcvkp::Image> FlatRenderPass::getColorImage()  { return nullptr; }

    FlatRenderPass::FlatRenderPass()
    {
        m_renderPass = std::make_shared<VkRenderPass>();
        for (size_t i = 0; i < VulkanGlobal::swapchainContext.swapChainImageViews.size(); i++)
        {
            m_swapChainFramebuffers.push_back(std::make_shared<VkFramebuffer>());
        }
        createRenderPass();
        createFramebuffers();
    }

    FlatRenderPass::~FlatRenderPass()
    {
        std::cout << "Destroying flat pass" << "\n";
        for (size_t i = 0; i < m_swapChainFramebuffers.size(); i++)
        {
            vkDestroyFramebuffer(VulkanGlobal::context.device, *m_swapChainFramebuffers[i], nullptr);
        }

        vkDestroyRenderPass(VulkanGlobal::context.device, *m_renderPass, nullptr);
    }

    void FlatRenderPass::createRenderPass()
    {
        // Color attachment for a framebuffer.
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = VulkanGlobal::swapchainContext.swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        // Clear the frame before render.
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        // Rendered contents will be stored in memory and can be read later.
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        // We don't care abot the initial layout because will draw on it
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // Attachment for a sub-pass.
        VkAttachmentReference colorAttachmentRef{};
        // Index of the attachement in the attachments array.
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // Attachment for dowsampling the final image.
        VkAttachmentDescription colorAttachmentResolve{};
        colorAttachmentResolve.format = VulkanGlobal::swapchainContext.swapChainImageFormat;
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentResolveRef{};
        colorAttachmentResolveRef.attachment = 1;
        colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        // subpass.pResolveAttachments = &colorAttachmentResolveRef;

        std::array<VkAttachmentDescription, 1> attachments = {colorAttachment};
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        // Subpass dependencies for layout transitions
        std::array<VkSubpassDependency, 2> dependencies;

        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        renderPassInfo.dependencyCount = 2;
        renderPassInfo.pDependencies = dependencies.data();

        if (vkCreateRenderPass(VulkanGlobal::context.device, &renderPassInfo, nullptr, m_renderPass.get()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void FlatRenderPass::createFramebuffers()
    {
        m_swapChainFramebuffers.resize(VulkanGlobal::swapchainContext.swapChainImageViews.size());
        for (size_t i = 0; i < VulkanGlobal::swapchainContext.swapChainImageViews.size(); i++)
        {
            std::array<VkImageView, 1> attachments = {
                VulkanGlobal::swapchainContext.swapChainImageViews[i]};

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = *m_renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = VulkanGlobal::swapchainContext.swapChainExtent.width;
            framebufferInfo.height = VulkanGlobal::swapchainContext.swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(VulkanGlobal::context.device, &framebufferInfo, nullptr, m_swapChainFramebuffers[i].get()) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }
}
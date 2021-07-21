#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <array>
#include "utils/vulkan.h"
#include "app-context/VulkanApplicationContext.h"
#include "app-context/VulkanGlobal.h"
#include "app-context/VulkanSwapchain.h"
#include "utils/RootDir.h"
#include "memory/VulkanBuffer.h"
#include "utils/glm.h"
#include "utils/Camera.h"
#include "scene/mesh.h"
#include "scene/ScreenQuadModel.h"
#include "compute/TextureOutputComputeModel.h"
#include "render-context/PostProcessRenderContext.h"
#include "pipeline/VulkanPipeline.h"
#include "pipeline/VulkanDescriptorSet.h"
// TODO: Organize includes!

const std::string path_prefix = std::string(ROOT_DIR) + "resources/";

float mouseOffsetX, mouseOffsetY;
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void processInput(GLFWwindow *window);

float deltaTime = 0.0f; // Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame
Camera camera(glm::vec3(1.8f, 8.6f, 1.1f));

bool hasMoved = false;
class HelloComputeApplication
{
public:
    void run()
    {
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    // Swapchain context - holds swapchain and its images and image views.
    VulkanSwapchain swapchainContext;

    // Compute model - holds render taget texture, descriptors and pipelines.
    TextureOutputComputeModel computeModel;

    // Post process render pass, has framebuffer for each swapchain image.
    PostProcessRenderContext postProcessRenderContext;

    // Layout for screen quad. Basically just a texture sampler for acessing rendered image.
    VkDescriptorSetLayout screenQuadDescriptorLayout;
    VkPipelineLayout screenQuadPipelineLayout;
    VkPipeline screenQuadPipeline;
    //Screen quad model - just a quad covering the screen.
    ScreenQuadVulkanModel screenQuadModel;

    // Command buffers for graphics.
    std::vector<VkCommandBuffer> commandBuffers;
    // Command buffers for compute.
    std::vector<VkCommandBuffer> computeCommandBuffers;

    // Synchronization objects.
    const int MAX_FRAMES_IN_FLIGHT = 2;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkSemaphore> computeSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;

    // Initializing layouts and models.
    void initScene()
    {
        computeModel.init(swapchainContext);

        VulkanDescriptorSet::screenQuadLayout(screenQuadDescriptorLayout);

        VulkanPipeline::createGraphicsPipeline(swapchainContext.swapChainExtent,
                                               &screenQuadDescriptorLayout,
                                               postProcessRenderContext.renderPass,
                                               path_prefix + "/shaders/generated/post-process-vert.spv",
                                               path_prefix + "/shaders/generated/post-process-frag.spv",
                                               screenQuadPipelineLayout,
                                               screenQuadPipeline);

        // Creating screen quad and passing color attachment of offscreen render pass as a texture.
        screenQuadModel.init(&screenQuadDescriptorLayout,
                             swapchainContext,
                             &computeModel.targetTexture);
    }

    uint32_t currentSample = 0;
    void updateScene(uint32_t currentImage)
    {

        float currentTime = (float)glfwGetTime();
        if (hasMoved)
        {
            currentSample = 0;
            hasMoved = false;
        }
        //std::cout << "Current sample: " << currentSample << std::endl;
        UniformBufferObject ubo = {camera.Position, currentTime, currentSample};
        computeModel.updateUniformBuffer(ubo, currentImage);
        currentSample++;
    }

    void createComputeCommandBuffers()
    {
        computeCommandBuffers.resize(VulkanGlobal::context.swapChainImageCount);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = VulkanGlobal::context.computeCommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)computeCommandBuffers.size();

        for (size_t i = 0; i < computeCommandBuffers.size(); i++)
        {
            if (vkAllocateCommandBuffers(VulkanGlobal::context.device, &allocInfo, &computeCommandBuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to allocate command buffers!");
            }

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = 0;                  // Optional
            beginInfo.pInheritanceInfo = nullptr; // Optional

            if (vkBeginCommandBuffer(computeCommandBuffers[i], &beginInfo) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to begin recording command buffer!");
            }

            VkImageMemoryBarrier imageMemoryBarrier = {};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            imageMemoryBarrier.image = computeModel.targetTexture.image;
            imageMemoryBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            imageMemoryBarrier.srcAccessMask = 0;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            vkCmdPipelineBarrier(
                computeCommandBuffers[i],
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &imageMemoryBarrier);

            vkCmdBindPipeline(computeCommandBuffers[i], VK_PIPELINE_BIND_POINT_COMPUTE, computeModel.pipeline);
            vkCmdBindDescriptorSets(computeCommandBuffers[i], VK_PIPELINE_BIND_POINT_COMPUTE, computeModel.pipelineLayout, 0, 1, &computeModel.descriptorSets[i], 0, 0);

            vkCmdDispatch(computeCommandBuffers[i], ceil(computeModel.targetTexture.width / 32.), ceil(computeModel.targetTexture.height / 32.), 1);

            //VulkanImage::cmdBlitTexture(computeCommandBuffers[i], computeModel.targetTexture, screenQuadModel.textureImage);

            if (vkEndCommandBuffer(computeCommandBuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to record command buffer!");
            }
        }
    }

    void createCommandBuffers()
    {
        commandBuffers.resize(VulkanGlobal::context.swapChainImageCount);
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = VulkanGlobal::context.graphicsCommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

        if (vkAllocateCommandBuffers(VulkanGlobal::context.device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate command buffers!");
        }

        for (size_t i = 0; i < commandBuffers.size(); i++)
        {

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = 0;                  // Optional
            beginInfo.pInheritanceInfo = nullptr; // Optional

            if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to begin recording command buffer!");
            }

            VkImageMemoryBarrier imageMemoryBarrier = {};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageMemoryBarrier.image = computeModel.targetTexture.image;
            imageMemoryBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            vkCmdPipelineBarrier(
                commandBuffers[i],
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &imageMemoryBarrier);

            std::array<VkClearValue, 2> clearValues{};
            clearValues[0].color = {1.0f, 0.5f, 1.0f, 1.0f};
            clearValues[1].depthStencil = {1.0f, 0};

            VkRenderPassBeginInfo postProcessRenderPassInfo{};
            postProcessRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            postProcessRenderPassInfo.renderPass = postProcessRenderContext.renderPass;
            postProcessRenderPassInfo.framebuffer = postProcessRenderContext.swapChainFramebuffers[i];
            postProcessRenderPassInfo.renderArea.offset = {0, 0};
            postProcessRenderPassInfo.renderArea.extent = swapchainContext.swapChainExtent;

            postProcessRenderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            postProcessRenderPassInfo.pClearValues = clearValues.data();
            vkCmdBeginRenderPass(commandBuffers[i], &postProcessRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, screenQuadPipeline);
            screenQuadModel.drawCommand(commandBuffers[i], screenQuadPipelineLayout, i);

            vkCmdEndRenderPass(commandBuffers[i]);
            if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to record command buffer!");
            }
        }
    }

    void createSyncObjects()
    {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        computeSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        imagesInFlight.resize(swapchainContext.swapChainImageViews.size());

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            if (vkCreateSemaphore(VulkanGlobal::context.device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(VulkanGlobal::context.device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(VulkanGlobal::context.device, &semaphoreInfo, nullptr, &computeSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(VulkanGlobal::context.device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
            {

                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }

    size_t currentFrame = 0;
    void drawFrame()
    {
        vkWaitForFences(VulkanGlobal::context.device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(VulkanGlobal::context.device,
                                                swapchainContext.swapChain,
                                                UINT64_MAX,
                                                imageAvailableSemaphores[currentFrame],
                                                VK_NULL_HANDLE,
                                                &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        // Check if a previous frame is using this image (i.e. there is its fence to wait on)
        if (imagesInFlight[imageIndex] != VK_NULL_HANDLE)
        {
            vkWaitForFences(VulkanGlobal::context.device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }
        // Mark the image as now being in use by this frame
        imagesInFlight[imageIndex] = inFlightFences[currentFrame];

        updateScene(imageIndex);
        /*
        if (currentSample  > 10 ) {
            return;
        }
        */
        VkSubmitInfo computeSubmitInfo{};
        computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        computeSubmitInfo.commandBufferCount = 1;
        computeSubmitInfo.pCommandBuffers = &computeCommandBuffers[imageIndex];
        computeSubmitInfo.waitSemaphoreCount = 0;

        VkSemaphore computeSignalSemaphores[] = {computeSemaphores[currentFrame]};

        computeSubmitInfo.signalSemaphoreCount = 1;
        computeSubmitInfo.pSignalSemaphores = computeSignalSemaphores;
        vkQueueSubmit(VulkanGlobal::context.computeQueue, 1, &computeSubmitInfo, VK_NULL_HANDLE);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore renderWaitSemaphores[] = {imageAvailableSemaphores[currentFrame], computeSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
        submitInfo.waitSemaphoreCount = 2;
        submitInfo.pWaitSemaphores = renderWaitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
        VkSemaphore renderSignalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = renderSignalSemaphores;

        vkResetFences(VulkanGlobal::context.device, 1, &inFlightFences[currentFrame]);

        if (vkQueueSubmit(VulkanGlobal::context.graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to submit draw command buffer!");
        }
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = renderSignalSemaphores;
        VkSwapchainKHR swapChains[] = {swapchainContext.swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr;

        result = vkQueuePresentKHR(VulkanGlobal::context.presentQueue, &presentInfo);

        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("failed to present swap chain image!");
        }

        // Commented this out for playing around with it later :)
        // vkQueueWaitIdle(VulkanGlobal::context.presentQueue);
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    int nbFrames = 0;
    float lastTime = 0;
    void mainLoop()
    {
        while (!glfwWindowShouldClose(VulkanGlobal::context.window))
        {
            float currentTime = (float)glfwGetTime();
            deltaTime = currentTime - lastFrame;
            nbFrames++;
            if (currentTime - lastTime >= 1.0)
            { // If last prinf() was more than 1 sec ago
                // printf and reset timer
                printf("%f ms/frame\n", 1000.0 / double(nbFrames));
                nbFrames = 0;
                lastTime = currentTime;
            }
            lastFrame = currentTime;

            processInput(VulkanGlobal::context.window);
            glfwPollEvents();
            drawFrame();
        }

        vkDeviceWaitIdle(VulkanGlobal::context.device);
    }

    void initVulkan()
    {
        swapchainContext.init();
        postProcessRenderContext.init(&swapchainContext);

        initScene();

        createCommandBuffers();
        createComputeCommandBuffers();
        createSyncObjects();
        glfwSetCursorPosCallback(VulkanGlobal::context.window, mouse_callback);
    }

    void cleanup()
    {
        vkQueueWaitIdle(VulkanGlobal::context.graphicsQueue);
        vkFreeCommandBuffers(VulkanGlobal::context.device, VulkanGlobal::context.graphicsCommandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
        vkQueueWaitIdle(VulkanGlobal::context.computeQueue);
        vkFreeCommandBuffers(VulkanGlobal::context.device, VulkanGlobal::context.computeCommandPool, static_cast<uint32_t>(computeCommandBuffers.size()), computeCommandBuffers.data());
        vkDestroyPipeline(VulkanGlobal::context.device, screenQuadPipeline, nullptr);
        vkDestroyPipelineLayout(VulkanGlobal::context.device, screenQuadPipelineLayout, nullptr);
        computeModel.destroy();

        postProcessRenderContext.destroy();
        swapchainContext.destroy();

        vkDestroyDescriptorSetLayout(VulkanGlobal::context.device, screenQuadDescriptorLayout, nullptr);
        screenQuadModel.destroy();

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(VulkanGlobal::context.device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(VulkanGlobal::context.device, imageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(VulkanGlobal::context.device, computeSemaphores[i], nullptr);
            vkDestroyFence(VulkanGlobal::context.device, inFlightFences[i], nullptr);
        }

        glfwTerminate();
    }
};

int main()
{
    HelloComputeApplication app;

    try
    {
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void processInput(GLFWwindow *window)
{
    Camera_Movement direction = NONE;
    hasMoved = false;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        direction = UP;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        direction = DOWN;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        direction = FORWARD;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        direction = BACKWARD;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        direction = LEFT;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        direction = RIGHT;

    if (direction != NONE)
    {
        camera.ProcessKeyboard(direction, deltaTime);
        hasMoved = true;
    }
}

float lastX = 400, lastY = 300;
bool firstMouse = true;
void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

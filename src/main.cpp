#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <array>
#include <memory>
#include "utils/vulkan.h"
#include "app-context/VulkanApplicationContext.h"
#include "app-context/VulkanSwapchain.h"
#include "app-context/VulkanGlobal.h"
#include "utils/RootDir.h"
#include "utils/glm.h"
#include "utils/Camera.h"
#include "scene/Mesh.h"
#include "scene/Scene.h"
#include "scene/DrawableModel.h"
#include "render-context/FlatRenderPass.h"
#include "render-context/RenderSystem.h"
#include "scene/ComputeMaterial.h"
#include "scene/ComputeModel.h"
#include "ray-tracing/RtScene.h"

// TODO: Organize includes!

#include <stdint.h>

#define u_int32_t uint32_t

const std::string path_prefix = std::string(ROOT_DIR) + "resources/";

float mouseOffsetX, mouseOffsetY;
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void processInput(GLFWwindow *window);

float deltaTime = 0.0f; // Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame
Camera camera(glm::vec3(1.8f, 8.6f, 1.1f));
bool hasMoved = false;
struct UniformBufferObject
{
    alignas(16) glm::vec3 camPosition;
    alignas(4) float time;
    alignas(4) u_int32_t currentSample;
    alignas(4) u_int32_t numTriangles;
};

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
    std::shared_ptr<GpuModel::Scene> rtScene;

    std::shared_ptr<mcvkp::ComputeModel> computeModel;

    std::shared_ptr<mcvkp::Scene> postProcessScene;

    std::vector<VkCommandBuffer> commandBuffers;

    const int MAX_FRAMES_IN_FLIGHT = 2;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;

    // Initializing layouts and models.
    void initScene()
    {
        using namespace mcvkp;
        uint32_t descriptorSetsSize = VulkanGlobal::swapchainContext.getImageViews().size();

        rtScene = std::make_shared<GpuModel::Scene>();

        // Buffer bundle is an array of buffers, one per each swapchain image/descriptor set.
        auto uniformBufferBundle = std::make_shared<mcvkp::BufferBundle>(descriptorSetsSize);
        BufferUtils::createBundle<UniformBufferObject>(uniformBufferBundle.get(), UniformBufferObject(),
                                                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        auto triangleBufferBundle = std::make_shared<mcvkp::BufferBundle>(descriptorSetsSize);
        BufferUtils::createBundle<GpuModel::Triangle>(triangleBufferBundle.get(), rtScene->triangles.data(), rtScene->triangles.size(),
                                                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        auto materialBufferBundle = std::make_shared<mcvkp::BufferBundle>(descriptorSetsSize);
        BufferUtils::createBundle<GpuModel::Material>(materialBufferBundle.get(), rtScene->materials.data(), rtScene->materials.size(),
                                                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        auto aabbBufferBundle = std::make_shared<mcvkp::BufferBundle>(descriptorSetsSize);
        BufferUtils::createBundle<GpuModel::BvhNode>(aabbBufferBundle.get(), rtScene->bvhNodes.data(), rtScene->bvhNodes.size(),
                                                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        auto lightsBufferBundle = std::make_shared<mcvkp::BufferBundle>(descriptorSetsSize);
        BufferUtils::createBundle<GpuModel::Light>(lightsBufferBundle.get(), rtScene->lights.data(), rtScene->lights.size(),
                                                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        auto spheresBufferBundle = std::make_shared<mcvkp::BufferBundle>(descriptorSetsSize);
        BufferUtils::createBundle<GpuModel::Sphere>(spheresBufferBundle.get(), rtScene->spheres.data(), rtScene->spheres.size(),
                                                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        auto targetTexture = std::make_shared<mcvkp::Image>();
        mcvkp::ImageUtils::createImage(VulkanGlobal::swapchainContext.getExtent().width,
                                       VulkanGlobal::swapchainContext.getExtent().height,
                                       1,
                                       VK_SAMPLE_COUNT_1_BIT,
                                       VK_FORMAT_R8G8B8A8_UNORM,
                                       VK_IMAGE_TILING_OPTIMAL,
                                       VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
                                       VK_IMAGE_ASPECT_COLOR_BIT,
                                       VMA_MEMORY_USAGE_GPU_ONLY,
                                       targetTexture);
        mcvkp::ImageUtils::transitionImageLayout(targetTexture->image,
                                                 VK_FORMAT_R8G8B8A8_UNORM,
                                                 VK_IMAGE_LAYOUT_UNDEFINED,
                                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                 1);

        // Uncomment to use a simplified shader.
        //auto computeMaterial = std::make_shared<ComputeMaterial>(path_prefix + "/shaders/generated/ray-trace-compute-simple.spv");
        auto computeMaterial = std::make_shared<ComputeMaterial>(path_prefix + "/shaders/generated/ray-trace-compute.spv");
        computeMaterial->addUniformBufferBundle(uniformBufferBundle, VK_SHADER_STAGE_COMPUTE_BIT);
        computeMaterial->addStorageImage(targetTexture, VK_SHADER_STAGE_COMPUTE_BIT);
        computeMaterial->addStorageBufferBundle(triangleBufferBundle, VK_SHADER_STAGE_COMPUTE_BIT);
        computeMaterial->addStorageBufferBundle(materialBufferBundle, VK_SHADER_STAGE_COMPUTE_BIT);
        computeMaterial->addStorageBufferBundle(aabbBufferBundle, VK_SHADER_STAGE_COMPUTE_BIT);
        computeMaterial->addStorageBufferBundle(lightsBufferBundle, VK_SHADER_STAGE_COMPUTE_BIT);
        computeMaterial->addStorageBufferBundle(spheresBufferBundle, VK_SHADER_STAGE_COMPUTE_BIT);
        computeModel = std::make_shared<ComputeModel>(computeMaterial);

        postProcessScene = std::make_shared<Scene>(RenderPassType::eFlat);

        auto screenTex = std::make_shared<Texture>(targetTexture);
        auto screenMaterial = std::make_shared<Material>(
            path_prefix + "/shaders/generated/post-process-vert.spv",
            path_prefix + "/shaders/generated/post-process-frag.spv");
        screenMaterial->addTexture(screenTex, VK_SHADER_STAGE_FRAGMENT_BIT);
        postProcessScene->addModel(std::make_shared<DrawableModel>(screenMaterial, MeshType::ePlane));
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
        UniformBufferObject ubo = {camera.Position, currentTime, currentSample, (uint32_t)rtScene->triangles.size()};

        auto &allocation = computeModel->getMaterial()->getUniformBufferBundles()[0].data->buffers[currentImage]->allocation;
        void *data;
        vmaMapMemory(VulkanGlobal::context.getAllocator(), allocation, &data);
        memcpy(data, &ubo, sizeof(ubo));
        vmaUnmapMemory(VulkanGlobal::context.getAllocator(), allocation);

        currentSample++;
    }

    void createCommandBuffers()
    {
        commandBuffers.resize(VulkanGlobal::swapchainContext.getImageViews().size());
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = VulkanGlobal::context.getCommandPool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

        auto tagetImage = computeModel->getMaterial()->getStorageImages()[0].data;
        if (vkAllocateCommandBuffers(VulkanGlobal::context.getDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
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

            // Convert image layout to GENERAL before writing into it in compute shader.
            VkImageMemoryBarrier computeMemoryBarrier = {};
            computeMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            computeMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            computeMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            computeMemoryBarrier.image = tagetImage->image;
            computeMemoryBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            computeMemoryBarrier.srcAccessMask = 0;
            computeMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

            vkCmdPipelineBarrier(
                commandBuffers[i],
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &computeMemoryBarrier);

            // Bind compute pipeline and dispatch compute command.
            computeModel->computeCommand(commandBuffers[i], i, tagetImage->width / 32, tagetImage->height / 32, 1);

            // Convert image layout to READ_ONLY_OPTIMAL before reading from it in fragment shader.
            VkImageMemoryBarrier screenQuadMemoryBarrier = {};
            screenQuadMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            screenQuadMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
            screenQuadMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            screenQuadMemoryBarrier.image = tagetImage->image;
            screenQuadMemoryBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            screenQuadMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            screenQuadMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(
                commandBuffers[i],
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &screenQuadMemoryBarrier);

            // Bind graphics pipeline and dispatch draw command.
            postProcessScene->writeRenderCommand(commandBuffers[i], i);

            if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to record command buffer!");
            }
        }
    }

    void createSyncObjects()
    {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        imagesInFlight.resize(VulkanGlobal::swapchainContext.getImageViews().size());

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            if (vkCreateSemaphore(VulkanGlobal::context.getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(VulkanGlobal::context.getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(VulkanGlobal::context.getDevice(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
            {

                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }

    size_t currentFrame = 0;
    void drawFrame()
    {
        vkWaitForFences(VulkanGlobal::context.getDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(VulkanGlobal::context.getDevice(),
                                                VulkanGlobal::swapchainContext.getBody(),
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
            vkWaitForFences(VulkanGlobal::context.getDevice(), 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }
        // Mark the image as now being in use by this frame
        imagesInFlight[imageIndex] = inFlightFences[currentFrame];

        updateScene(imageIndex);
        vkResetFences(VulkanGlobal::context.getDevice(), 1, &inFlightFences[currentFrame]);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore renderWaitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = renderWaitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
        VkSemaphore renderSignalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = renderSignalSemaphores;

        if (vkQueueSubmit(VulkanGlobal::context.getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to submit draw command buffer!");
        }
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = renderSignalSemaphores;
        VkSwapchainKHR swapChains[] = {VulkanGlobal::swapchainContext.getBody()};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr;

        result = vkQueuePresentKHR(VulkanGlobal::context.getPresentQueue(), &presentInfo);

        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("failed to present swap chain image!");
        }

        // Commented this out for playing around with it later :)
        // vkQueueWaitIdle(VulkanGlobal::context.getPresentQueue());
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    int nbFrames = 0;
    float lastTime = 0;
    void mainLoop()
    {
        while (!glfwWindowShouldClose(VulkanGlobal::context.getWindow()))
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

            processInput(VulkanGlobal::context.getWindow());
            glfwPollEvents();
            drawFrame();
        }

        vkDeviceWaitIdle(VulkanGlobal::context.getDevice());
    }

    void initVulkan()
    {
        initScene();

        createCommandBuffers();
        createSyncObjects();
        //glfwSetCursorPosCallback(VulkanGlobal::context.getWindow(), mouse_callback);
    }

    void cleanup()
    {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(VulkanGlobal::context.getDevice(), renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(VulkanGlobal::context.getDevice(), imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(VulkanGlobal::context.getDevice(), inFlightFences[i], nullptr);
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

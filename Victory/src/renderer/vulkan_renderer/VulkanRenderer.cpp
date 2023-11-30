#include <vulkan/vulkan.h>

#include "VulkanRenderer.h"

#include <GLFW/glfw3.h>
#include <stdexcept>
#include <stdio.h>
#include <array>

#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanPipeline.h"
#include "VulkanBuffer.h"
#include "VulkanFrameBuffer.h"
#include "VulkanUtils.h"

#define CHK_RESULT(RESULT, MESSAGE) \
    if (RESULT == false) { \
        throw std::runtime_error(MESSAGE); \
    } 

void FrameBufferResizeCallback(GLFWwindow *window_, int width_, int height_) {
    (void)width_;
    (void)height_;
    auto app = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window_));
    app->SetIsResized(true);
}

Renderer* VulkanRenderer::CreateRenderer() {
    return new VulkanRenderer();
}

void VulkanRenderer::Initialize(const char *applicationName_){
    printf("\nVulkanRenderer::Initialize");
    if (!glfwInit()) {
        throw std::runtime_error("GLFW was not created!");
    };

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_Window = glfwCreateWindow(1280, 720, "Victory", nullptr, nullptr);
    if (!m_Window) {
        glfwTerminate();
        throw std::runtime_error("GLFW Window was not created!");
    }

    glfwSetWindowUserPointer(m_Window, this);
    glfwSetFramebufferSizeCallback(m_Window, FrameBufferResizeCallback);

    m_VulkanContext = new VulkanContext();

    CHK_RESULT(m_VulkanContext->CreateInstance(applicationName_), 
        "Insance was not created!");

#ifndef NDEBUG
    CHK_RESULT(m_VulkanContext->RegisterDebugUtilsMessenger(), 
        "Debug messenger was not created!");
#endif

    m_VulkanSwapchain = new VulkanSwapchain(m_VulkanContext);

    CHK_RESULT(m_VulkanSwapchain->CreateSurface(m_Window),
        "Surface was not created!");

    CHK_RESULT(m_VulkanContext->PickPhysicalDevice(m_VulkanSwapchain->GetSurface()), 
        "Physical Device was not picked!");

    CHK_RESULT(m_VulkanContext->CreateLogicalDevice(), 
        "Logical Device was not created!");

    CHK_RESULT(m_VulkanSwapchain->CreateSwapchain(m_Window),
        "Swapchain was not created!");

    CHK_RESULT(m_VulkanSwapchain->CreateImageViews(), 
        "Image views were not created!");

    m_VulkanPipeline = new VulkanPipeline(m_VulkanContext);

    CHK_RESULT(m_VulkanPipeline->CreateRenderPass(m_VulkanSwapchain->GetSurfaceFormat().format),
        "Render pass was not created!");

    CHK_RESULT(m_VulkanPipeline->CreateDescriptorSetLayout(),
        "Descriptor set was not created");
    
    CHK_RESULT(m_VulkanPipeline->CreatePipelineLayout(),
        "Pipeline layout was not created!");

    CHK_RESULT(m_VulkanPipeline->CreatePipeline(),
        "Pipeline was not created!");

    // TODO: Get rid of cross ref
    m_VulkanFrameBuffer = new VulkanFrameBuffer(m_VulkanContext, m_VulkanSwapchain, m_VulkanPipeline, m_VulkanBuffer);
    m_VulkanBuffer = new VulkanBuffer(m_VulkanContext, m_VulkanPipeline, m_VulkanFrameBuffer, m_VulkanSwapchain);
    m_VulkanFrameBuffer->SetVulkanBuffer(m_VulkanBuffer);

    CHK_RESULT(m_VulkanBuffer->CreateDepthResources(),
        "Depth resources were not created");

    CHK_RESULT(m_VulkanBuffer->LoadModel(),
        "Model was not loaded");

    CHK_RESULT(m_VulkanFrameBuffer->CreateFrameBuffers(),
        "Frame buffers were not created!");

    CHK_RESULT(m_VulkanFrameBuffer->CreateCommandPool(),
        "Command pool was not created!");

    CHK_RESULT(m_VulkanBuffer->CreateTextureImage(), 
        "Texture image was not created");

    CHK_RESULT(m_VulkanBuffer->CreateTextureImageView(), 
        "Texture image view was not created");

    CHK_RESULT(m_VulkanBuffer->CreateTextureSampler(), 
        "Texture sampler view was not created");

    CHK_RESULT(m_VulkanBuffer->CreateVertexBuffer(),
        "Vertex buffer was not created");

    CHK_RESULT(m_VulkanBuffer->CreateIndexBuffer(),
        "Index buffer was not created");

    CHK_RESULT(m_VulkanBuffer->CreateUniformBuffers(MAX_FRAMES_IN_FLIGHT),
        "Uniform buffer was not created");

    CHK_RESULT(m_VulkanBuffer->CreateDescriptorPool(MAX_FRAMES_IN_FLIGHT),
        "Descriptor pool was not created");

    CHK_RESULT(m_VulkanBuffer->CreateDescriptorSets(MAX_FRAMES_IN_FLIGHT),
        "Descriptor sets was not created");

    CHK_RESULT(m_VulkanFrameBuffer->CreateCommandBuffer(MAX_FRAMES_IN_FLIGHT),
        "Command buffer was not created!");

    m_AvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_FinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    for (uint32_t i{0}; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        CHK_RESULT(VulkanUtils::CreateSemaphore(m_VulkanContext->GetDevice(), &m_AvailableSemaphores[i]),
            "Available semaphore was not created!");
        CHK_RESULT(VulkanUtils::CreateSemaphore(m_VulkanContext->GetDevice(), &m_FinishedSemaphores[i]),
            "Finished semaphore was not created!");
        CHK_RESULT(VulkanUtils::CreateFence(m_VulkanContext->GetDevice(), &m_InFlightFences[i]),
            "Fence was not created!");
    }
}

bool VulkanRenderer::IsRunning() {
    printf("\nVulkanRenderer::IsRunning");
    return !glfwWindowShouldClose(m_Window);
}

void VulkanRenderer::PollEvents() {
    printf("\nVulkanRenderer::PollEvents");
    glfwPollEvents();
}

bool VulkanRenderer::Resize() {
    printf("\nVulkanRenderer::Resize");

    VkDevice device = m_VulkanContext->GetDevice();
    CHK_RESULT((vkWaitForFences(device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX) == VK_SUCCESS),
        "Wait for fences = false!");

    VkResult acquireResult = vkAcquireNextImageKHR(device, m_VulkanSwapchain->GetSwapchain(), UINT64_MAX, m_AvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &m_ImageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR || acquireResult == VK_SUBOPTIMAL_KHR || m_IsResized)
    {
        m_IsResized = false;
        RecreateSwapchain();
        vkDestroySemaphore(device, m_AvailableSemaphores[m_CurrentFrame], nullptr);
        VulkanUtils::CreateSemaphore(device, &m_AvailableSemaphores[m_CurrentFrame]);
        return true;
    }
    else if (acquireResult != VK_SUCCESS)
    {
        throw std::runtime_error("Failde to acquire swap chain image!");
    }
    return false;
}

void VulkanRenderer::BeginFrame() {
    printf("\nVulkanRenderer::BeginFrame");

    m_VulkanBuffer->UpdateUniformBuffer(m_CurrentFrame);
    vkResetFences(m_VulkanContext->GetDevice(), 1, &m_InFlightFences[m_CurrentFrame]);
}

void VulkanRenderer::RecordCommandBuffer() {
    VkCommandBuffer commandBuffer = m_VulkanFrameBuffer->GetCommandBuffer(m_CurrentFrame);
    vkResetCommandBuffer(commandBuffer, 0);

    VkCommandBufferBeginInfo commandBufferBI{};
    commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    vkBeginCommandBuffer(commandBuffer, &commandBufferBI);

    VkRect2D rect{};
    rect.offset = {0, 0};
    rect.extent = m_VulkanSwapchain->GetExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{48.f / 255.f, 10.f / 255.f, 36.f / 255.f, 1.f}};
    clearValues[1].depthStencil = {1.f, 0};

    VkRenderPassBeginInfo renderPassBI{};
    renderPassBI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBI.renderPass = m_VulkanPipeline->GetRenderPass();
    renderPassBI.framebuffer = m_VulkanFrameBuffer->GetFrameBuffer(m_ImageIndex);
    renderPassBI.renderArea = rect;
    renderPassBI.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBI.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VulkanPipeline->GetPipeline());

        // TODO: split vertex/index data from m_Buffer
        std::vector<VkBuffer> vertexBuffers{m_VulkanBuffer->GetVertexBuffer()};
        VkBuffer indexBuffer{m_VulkanBuffer->GetIndexBuffer()};
        std::vector<VkDeviceSize> offsets{0};

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets.data());
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        VkViewport viewport{};
        viewport.x = 0.f;
        viewport.y = 0.f;
        viewport.width = static_cast<float>(rect.extent.width);
        viewport.height = static_cast<float>(rect.extent.height);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &rect);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VulkanPipeline->GetPipelineLayout(), 
            0, 1, &m_VulkanBuffer->GetDescriptorSet(m_CurrentFrame), 0, nullptr);

        vkCmdDrawIndexed(commandBuffer, m_VulkanBuffer->GetIndicesCount(), 1, 0, 0, 0);

    }
    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);
}

void VulkanRenderer::EndFrame() {
    const std::vector<VkSemaphore> waitSemaphores{m_AvailableSemaphores[m_CurrentFrame]};
    const std::vector<VkSemaphore> signalSemaphores{m_FinishedSemaphores[m_CurrentFrame]};
    const std::vector<VkPipelineStageFlags> waitStages{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkCommandBuffer commandBuffer = m_VulkanFrameBuffer->GetCommandBuffer(m_CurrentFrame);
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.pWaitDstStageMask = waitStages.data();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores.data();

    VkQueue graphicsQueue = m_VulkanContext->GetQueue(QueueIndex::eGraphics);
    CHK_RESULT((vkQueueSubmit(graphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrame]) == VK_SUCCESS),
        "Failed to submit draw command buffer!");

    const std::vector<VkSwapchainKHR> swapOld{m_VulkanSwapchain->GetSwapchain()};
    const std::vector<VkSemaphore> signalOld{m_FinishedSemaphores[m_CurrentFrame]};

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = static_cast<uint32_t>(signalOld.size());
    presentInfo.pWaitSemaphores = signalOld.data();
    presentInfo.swapchainCount = static_cast<uint32_t>(swapOld.size());
    presentInfo.pSwapchains = swapOld.data();
    presentInfo.pImageIndices = &m_ImageIndex;

    VkQueue presentQueue = m_VulkanContext->GetQueue(QueueIndex::ePresent);

    // TODO: Resize by changing window size
    VkResult presentResult = vkQueuePresentKHR(presentQueue, &presentInfo);
    if (presentResult ==  VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || m_IsResized) {
        Resize();
    } else if (presentResult != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    printf("\nVulkanRenderer::EndFrame");
}

void VulkanRenderer::Destroy() {
    VkDevice device = m_VulkanContext->GetDevice();

    vkDeviceWaitIdle(device);

    for (size_t i{0}, n = m_AvailableSemaphores.size(); i < n; ++i) {
        vkDestroySemaphore(device, m_AvailableSemaphores[i], nullptr);
        vkDestroySemaphore(device, m_FinishedSemaphores[i], nullptr);
        vkDestroyFence(device, m_InFlightFences[i], nullptr);
    }

    m_VulkanBuffer->CleanupAll();
    m_VulkanFrameBuffer->CleanupAll();
    m_VulkanPipeline->CleanupAll();
    m_VulkanSwapchain->CleanupAll();
    m_VulkanContext->CleanupAll();

    glfwDestroyWindow(m_Window);
    glfwTerminate();

    delete m_VulkanBuffer;
    delete m_VulkanFrameBuffer;
    delete m_VulkanPipeline;
    delete m_VulkanSwapchain;
    delete m_VulkanContext;
    printf("\nVulkanRenderer::Destroy\n");
}

void VulkanRenderer::SetIsResized(bool isResized_) {
    m_IsResized = isResized_;
}

void VulkanRenderer::RecreateSwapchain() {
    // TODO: doesn't work on UNIX
    int width = 0, height = 0;
    glfwGetFramebufferSize(m_Window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(m_Window, &width, &height);
        printf("\n %i%i", width,height);
        glfwWaitEvents();
    }

    VkDevice device = m_VulkanContext->GetDevice();
    vkDeviceWaitIdle(device);

    m_VulkanSwapchain->CleanupSwapchain();
    m_VulkanSwapchain->CleanupImageViews();
    m_VulkanBuffer->CleanupDepthResources();
    m_VulkanFrameBuffer->CleanupFrameBuffers();

    CHK_RESULT(m_VulkanSwapchain->CreateSwapchain(m_Window),
        "Swapchain was not created!");

    CHK_RESULT(m_VulkanSwapchain->CreateImageViews(), 
        "Image views were not created!");

    CHK_RESULT(m_VulkanBuffer->CreateDepthResources(), 
        "Depth Resources were not created!");

    CHK_RESULT(m_VulkanFrameBuffer->CreateFrameBuffers(),
        "Frame buffers were not created!");
}
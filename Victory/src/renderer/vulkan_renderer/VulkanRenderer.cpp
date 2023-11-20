#include "VulkanRenderer.h"

#include <GLFW/glfw3.h>
#include "VulkanSynchronization.h"

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

    CHK_RESULT(m_VulkanContext.CreateInstance(applicationName_), 
        "Insance was not created!");

#ifndef NDEBUG
    CHK_RESULT(m_VulkanContext.RegisterDebugUtilsMessenger(), 
        "Debug messenger was not created!");
#endif

    CHK_RESULT(m_VulkanSwapchain.CreateSurface(m_VulkanContext.GetInstance(), m_Window),
        "Surface was not created!");

    CHK_RESULT(m_VulkanContext.PickPhysicalDevice(m_VulkanSwapchain.GetSurface()), 
        "Physical Device was not picked!");

    CHK_RESULT(m_VulkanContext.CreateLogicalDevice(), 
        "Logical Device was not created!");

    CHK_RESULT(m_VulkanSwapchain.CreateSwapchain(m_VulkanContext, m_Window),
        "Swapchain was not created!");

    CHK_RESULT(m_VulkanSwapchain.CreateImageViews(m_VulkanContext), 
        "Image views were not created!");

    CHK_RESULT(m_VulkanPipeline.CreateRenderPass(m_VulkanContext.GetDevice(), m_VulkanSwapchain.GetSurfaceFormat().format),
        "Render pass was not created!");
    
    CHK_RESULT(m_VulkanPipeline.CreatePipelineLayout(m_VulkanContext.GetDevice()),
        "Pipeline layout was not created!");

    CHK_RESULT(m_VulkanPipeline.CreatePipeline(m_VulkanContext),
        "Pipeline was not created!");

    CHK_RESULT(m_VulkanFrameBuffer.CreateFrameBuffers(m_VulkanContext, m_VulkanSwapchain, m_VulkanPipeline),
        "Frame buffers were not created!");

    CHK_RESULT(m_VulkanFrameBuffer.CreateCommandPool(m_VulkanContext),
        "Command pool was not created!");

    CHK_RESULT(m_VertexBuffer.CreateVertexBuffer(m_VulkanContext.GetDevice()),
        "Vertex buffer was not created");
    
    CHK_RESULT(m_VertexBuffer.MemoryAllocation(m_VulkanContext),
        "Memory for vertex buffer was not allocated");
    
    CHK_RESULT(m_VulkanFrameBuffer.CreateCommandBuffer(m_VulkanContext.GetDevice(), MAX_FRAMES_IN_FLIGHT),
        "Command buffer was not created!");

    m_AvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_FinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    for (uint32_t i{0}; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        CHK_RESULT(CreateSemaphore(m_VulkanContext.GetDevice(), &m_AvailableSemaphores[i]),
            "Available semaphore was not created!");
        CHK_RESULT(CreateSemaphore(m_VulkanContext.GetDevice(), &m_FinishedSemaphores[i]),
            "Finished semaphore was not created!");
        CHK_RESULT(CreateFence(m_VulkanContext.GetDevice(), &m_InFlightFences[i]),
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

void VulkanRenderer::Resize() {
    printf("\nVulkanRenderer::Resize");

    // TODO: doesn't work on UNIX
    int width = 0, height = 0;
    glfwGetFramebufferSize(m_Window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(m_Window, &width, &height);
        printf("\n %i%i", width,height);
        glfwWaitEvents();
    }

    VkDevice device = m_VulkanContext.GetDevice();
    vkDeviceWaitIdle(device);

    m_VulkanSwapchain.CleanupSwapchain(device);
    m_VulkanSwapchain.CleanupImageViews(device);
    m_VulkanFrameBuffer.CleanupFrameBuffers(device);

    CHK_RESULT(m_VulkanSwapchain.CreateSwapchain(m_VulkanContext, m_Window),
        "Swapchain was not created!");

    CHK_RESULT(m_VulkanSwapchain.CreateImageViews(m_VulkanContext), 
        "Image views were not created!");

    CHK_RESULT(m_VulkanFrameBuffer.CreateFrameBuffers(m_VulkanContext, m_VulkanSwapchain, m_VulkanPipeline),
        "Frame buffers were not created!");
}

void VulkanRenderer::BeginFrame() {
    printf("\nVulkanRenderer::BeginFrame");

    vk::Device device = m_VulkanContext.GetDevice();
    CHK_RESULT((vkWaitForFences(device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX) == VK_SUCCESS),
        "Wait for fences = false!");

    uint32_t imageIndex{0};
    VkResult acquireResult = vkAcquireNextImageKHR(device, m_VulkanSwapchain.GetSwapchain(), UINT64_MAX, m_AvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR || acquireResult == VK_SUBOPTIMAL_KHR || m_IsResized) {
        m_IsResized = false;
        Resize();
        vkDestroySemaphore(device, m_AvailableSemaphores[m_CurrentFrame], nullptr);
        CreateSemaphore(device, &m_AvailableSemaphores[m_CurrentFrame]);
        return;
    } else if (acquireResult != VK_SUCCESS) {
        throw std::runtime_error("Failde to acquire swap chain image!");
    }

    vkResetFences(device, 1, &m_InFlightFences[m_CurrentFrame]);

    VkCommandBuffer commandBuffer = m_VulkanFrameBuffer.GetCommandBuffer(m_CurrentFrame);
    vkResetCommandBuffer(commandBuffer, 0);

    m_VulkanFrameBuffer.RecordCommandBuffer(m_VulkanSwapchain, m_VulkanPipeline, m_CurrentFrame, imageIndex, m_VertexBuffer);

    const std::vector<VkSemaphore> waitSemaphores{m_AvailableSemaphores[m_CurrentFrame]};
    const std::vector<VkSemaphore> signalSemaphores{m_FinishedSemaphores[m_CurrentFrame]};
    const std::vector<VkPipelineStageFlags> waitStages{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.pWaitDstStageMask = waitStages.data();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores.data();

    VkQueue graphicsQueue = m_VulkanContext.GetQueue(QueueIndex::eGraphics);
    CHK_RESULT((vkQueueSubmit(graphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrame]) == VK_SUCCESS),
        "Failed to submit draw command buffer!");

    const std::vector<VkSwapchainKHR> swapOld{m_VulkanSwapchain.GetSwapchain()};
    const std::vector<VkSemaphore> signalOld{m_FinishedSemaphores[m_CurrentFrame]};

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = static_cast<uint32_t>(signalOld.size());
    presentInfo.pWaitSemaphores = signalOld.data();
    presentInfo.swapchainCount = static_cast<uint32_t>(swapOld.size());
    presentInfo.pSwapchains = swapOld.data();
    presentInfo.pImageIndices = &imageIndex;

    VkQueue presentQueue = m_VulkanContext.GetQueue(QueueIndex::ePresent);

    // TODO: Resize by changing window size
    VkResult presentResult = vkQueuePresentKHR(presentQueue, &presentInfo);
    if (presentResult ==  VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || m_IsResized) {
        Resize();
    } else if (presentResult != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::EndFrame() {
    printf("\nVulkanRenderer::EndFrame");
}

void VulkanRenderer::Destroy() {
    VkDevice device = m_VulkanContext.GetDevice();

    vkDeviceWaitIdle(device);

    for (size_t i{0}, n = m_AvailableSemaphores.size(); i < n; ++i) {
        vkDestroySemaphore(device, m_AvailableSemaphores[i], nullptr);
        vkDestroySemaphore(device, m_FinishedSemaphores[i], nullptr);
        vkDestroyFence(device, m_InFlightFences[i], nullptr);
    }

    m_VertexBuffer.CleanupAll(m_VulkanContext.GetDevice());
    m_VulkanFrameBuffer.CleanupAll(m_VulkanContext);
    m_VulkanPipeline.CleanupAll(m_VulkanContext);
    m_VulkanSwapchain.CleanupAll(m_VulkanContext);
    m_VulkanContext.CleanupAll();
    printf("\nVulkanRenderer::Destroy\n");
}

void VulkanRenderer::SetIsResized(bool isResized_) {
    m_IsResized = isResized_;
}

#include "VulkanRenderer.h"
#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanPipeline.h"
#include "VulkanFrameBuffer.h"
#include "VulkanSynchronization.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <stdio.h>

// TODO: Change from new to static members
VulkanRenderer::VulkanRenderer()
 : m_VulkanContext(new VulkanContext())
 , m_VulkanSwapchain(new VulkanSwapchain())
 , m_VulkanPipeline(new VulkanPipeline()) 
 , m_VulkanFrameBuffer(new VulkanFrameBuffer()) { }

VulkanRenderer::~VulkanRenderer() {
    Destroy();

    delete m_VulkanFrameBuffer;
    delete m_VulkanPipeline;
    delete m_VulkanSwapchain;
    delete m_VulkanContext;

    printf("\nVulkanRenderer::~");
}

bool VulkanRenderer::IsRunning() {
    return !glfwWindowShouldClose(m_Window);
}

void VulkanRenderer::PollEvents() {
    glfwPollEvents();
}

void VulkanRenderer::Resize() {
    printf("\nVulkanRenderer::Resize");
}

void VulkanRenderer::BeginFrame() {
    printf("\nVulkanRenderer::BeginFrame");

    vk::Device device = m_VulkanContext->GetDevice();
    if (device.waitForFences(m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX) != vk::Result::eSuccess) {
        throw std::runtime_error("Wait for fences = false");
    }
    device.resetFences(m_InFlightFences[m_CurrentFrame]);

    uint32_t imageIndex = device.acquireNextImageKHR(m_VulkanSwapchain->GetSwapchain(), UINT64_MAX, m_AvailableSemaphores[m_CurrentFrame]).value;

    vk::CommandBuffer commandBuffer = m_VulkanFrameBuffer->GetCommandBuffer(m_CurrentFrame);
    commandBuffer.reset();

    m_VulkanFrameBuffer->RecordCommandBuffer(m_VulkanSwapchain, m_VulkanPipeline, m_CurrentFrame, imageIndex);

    std::vector<vk::Semaphore> waitSemaphores{m_AvailableSemaphores[m_CurrentFrame]};
    std::vector<vk::Semaphore> signalSemaphores{m_FinishedSemaphores[m_CurrentFrame]};
    std::vector<vk::PipelineStageFlags> waitStages{vk::PipelineStageFlagBits::eColorAttachmentOutput};

    vk::SubmitInfo submitInfo{};
    submitInfo.setWaitSemaphores(waitSemaphores);
    submitInfo.setWaitDstStageMask(waitStages);
    submitInfo.setCommandBuffers(commandBuffer);
    submitInfo.setSignalSemaphores(signalSemaphores);

    vk::Queue graphicsQueue = device.getQueue(m_VulkanContext->GetQueueIndex(0), 0);
    graphicsQueue.submit(submitInfo, m_InFlightFences[m_CurrentFrame]);

    vk::SwapchainKHR swaps = m_VulkanSwapchain->GetSwapchain();
    std::vector<vk::SwapchainKHR> swapchains = {swaps};

    vk::PresentInfoKHR presentInfo{};
    presentInfo.setWaitSemaphores(signalSemaphores);
    presentInfo.setSwapchains(swaps);
    presentInfo.setImageIndices(imageIndex);

    vk::Queue presentQueue = device.getQueue(m_VulkanContext->GetQueueIndex(1), 0);
    if (presentQueue.presentKHR(presentInfo) != vk::Result::eSuccess) {
        throw std::runtime_error("Not presented");
    }

    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::EndFrame() {
    m_VulkanContext->GetDevice().waitIdle();
    printf("\nVulkanRenderer::EndFrame");
}

void VulkanRenderer::Destroy() {
    for (int i = static_cast<int>(MAX_FRAMES_IN_FLIGHT) - 1; i >= 0; --i) {
        m_VulkanContext->GetDevice().destroySemaphore(m_AvailableSemaphores[i]);
        m_VulkanContext->GetDevice().destroySemaphore(m_FinishedSemaphores[i]);
        m_VulkanContext->GetDevice().destroyFence(m_InFlightFences[i]);
    }

    m_VulkanFrameBuffer->Cleanup(m_VulkanContext);
    m_VulkanPipeline->Cleanup(m_VulkanContext);
    m_VulkanSwapchain->Cleanup(m_VulkanContext);
    m_VulkanContext->Cleanup();

    glfwDestroyWindow(m_Window);
    glfwTerminate();
    printf("\nVulkanRenderer::Destroy");
}

bool VulkanRenderer::Initialize(const char* applicationName_) {
    printf("\nVulkanRenderer::Initialize");
    if (!glfwInit()) {
        printf("\nGLFW not inited");
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_Window = glfwCreateWindow(1280, 720, "Victory", nullptr, nullptr);
    if (!m_Window) {
        glfwTerminate();
        printf("\nGLFW window not inited");
        return false;
    }

    if (!m_VulkanContext->CreateInstance(applicationName_)) {
        return false;
    }

#ifndef NDEBUG
    m_VulkanContext->RegisterDebugUtilsMessenger();
#endif // NDEBUG

    if (!m_VulkanSwapchain->CreateSurface(m_VulkanContext, m_Window)) {
        return false;
    }

    if (!m_VulkanContext->PickPhysicalDevice(m_VulkanSwapchain->GetSurface())) {
        return false;
    }

    if (!m_VulkanContext->CreateLogicalDevice()) {
        return false;
    }

    if (!m_VulkanSwapchain->CreateSwapchain(m_VulkanContext, m_Window)) {
        return false;
    }

    if (!m_VulkanSwapchain->CreateImageViews(m_VulkanContext)) {
        return false;
    }

    if (!m_VulkanPipeline->CreateRenderPass(m_VulkanContext->GetDevice(), m_VulkanSwapchain->GetSwapchainFormat())) {
        return false;
    }

    if (!m_VulkanPipeline->CreatePipelineLayout(m_VulkanContext->GetDevice())) {
        return false;
    }

    if (!m_VulkanPipeline->CreatePipeline(m_VulkanContext)) {
        return false;
    }

    // Vulkan not executed directly using function calls
    // You have to record all of the operations in command buffer
    if (!m_VulkanFrameBuffer->CreateFrameBuffer(m_VulkanContext, m_VulkanSwapchain, m_VulkanPipeline)) {
        return false;
    }

    // Command pool acts as a container for command buffers,
    // that manages the allocation and management of command buffers
    if (!m_VulkanFrameBuffer->CreateCommandPool(m_VulkanContext)) {
        return false;
    }

    // command buffer used to record commands 
    // that are submitted to the GPU for execution
    if (!m_VulkanFrameBuffer->CreateCommandBuffer(m_VulkanContext, MAX_FRAMES_IN_FLIGHT)) {
        return false;
    }

    m_AvailableSemaphores.reserve(MAX_FRAMES_IN_FLIGHT);
    m_FinishedSemaphores.reserve(MAX_FRAMES_IN_FLIGHT);
    m_InFlightFences.reserve(MAX_FRAMES_IN_FLIGHT);

    m_AvailableSemaphores = {CreateSemaphore(m_VulkanContext->GetDevice()), CreateSemaphore(m_VulkanContext->GetDevice())};
    m_FinishedSemaphores = {CreateSemaphore(m_VulkanContext->GetDevice()), CreateSemaphore(m_VulkanContext->GetDevice())};
    m_InFlightFences = {CreateFence(m_VulkanContext->GetDevice()), CreateFence(m_VulkanContext->GetDevice())};

    return true;
}

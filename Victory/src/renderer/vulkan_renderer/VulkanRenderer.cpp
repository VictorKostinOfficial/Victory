#include "VulkanRenderer.h"
#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanPipeline.h"
#include "VulkanFrameBuffers.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <stdio.h>

// TODO: Change new on static members
VulkanRenderer::VulkanRenderer()
 : m_VulkanContext(new VulkanContext())
 , m_VulkanSwapchain(new VulkanSwapchain())
 , m_VulkanPipeline(new VulkanPipeline()) {
}

VulkanRenderer::~VulkanRenderer()
{
    Destroy();

    delete m_VulkanPipeline;
    delete m_VulkanSwapchain;
    delete m_VulkanContext;

    printf("\nVulkanRenderer::~");
}

void VulkanRenderer::Resize() {
    printf("\nVulkanRenderer::Resize");
}

void VulkanRenderer::BeginFrame() {
    printf("\nVulkanRenderer::BeginFrame");
}

void VulkanRenderer::EndFrame() {
    printf("\nVulkanRenderer::EndFrame");
}

void VulkanRenderer::Destroy() {
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

    if (!m_VulkanPipeline->CreateRenderPass(m_VulkanContext->GetDevice(), m_VulkanSwapchain->GetSwapchainFormat())) {
        return false;
    }

    if (!m_VulkanPipeline->CreatePipelineLayout(m_VulkanContext->GetDevice())) {
        return false;
    }

    if (!m_VulkanPipeline->CreatePipeline(m_VulkanContext, m_VulkanSwapchain)) {
        return false;
    }

    if (!m_VulkanFrameBuffers->CreateFrameBuffers(m_VulkanContext)) {
        return false;
    }

    return true;
}

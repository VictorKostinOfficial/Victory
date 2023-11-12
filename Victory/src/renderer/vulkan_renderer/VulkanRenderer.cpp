#include "VulkanRenderer.h"
#include "VulkanContext.h"
#include "VulkanSwapchain.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <stdio.h>

// TODO: Change new on static members
VulkanRenderer::VulkanRenderer()
 : m_VulkanContext(new VulkanContext())
 , m_VulkanSwapcahin(new VulkanSwapchain()) {
}

VulkanRenderer::~VulkanRenderer()
{
    Destroy();

    delete m_VulkanSwapcahin;
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
    m_VulkanSwapcahin->Cleanup(m_VulkanContext);
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

    if (!m_VulkanSwapcahin->CreateSurface(m_VulkanContext, m_Window)) {
        return false;
    }

    if (!m_VulkanContext->PickPhysicalDevice(m_VulkanSwapcahin->GetSurface())) {
        return false;
    }

    if (!m_VulkanContext->CreateLogicalDevice()) {
        return false;
    }
    
    return true;
}

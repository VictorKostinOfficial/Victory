#include "VulkanRenderer.h"
#include "VulkanContext.h"

#include <stdio.h>

VulkanRenderer::~VulkanRenderer() {
    Destroy();
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
    m_VulkanContext->Cleanup();

    delete m_VulkanContext;
    printf("\nVulkanRenderer::Destroy");
}

void VulkanRenderer::Initialize(const char* applicationName) {
    printf("\nVulkanRenderer::Initialize");
    m_VulkanContext = new VulkanContext();
    if (!m_VulkanContext->Initialize(applicationName)) {
        delete m_VulkanContext;
        throw std::runtime_error("Vulkan not inited");
    }
}

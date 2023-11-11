#include "VulkanRenderer.h"
#include "VulkanContext.h"

#include <stdio.h>

VulkanRenderer::~VulkanRenderer() {
    Destroy();
    printf("VulkanRenderer::~\n");
}

void VulkanRenderer::Resize() {
    printf("VulkanRenderer::Resize\n");
}

void VulkanRenderer::BeginFrame() {
    printf("VulkanRenderer::BeginFrame\n");
}

void VulkanRenderer::EndFrame() {
    printf("VulkanRenderer::EndFrame\n");
}

void VulkanRenderer::Destroy() {
    m_VulkanContext->Cleanup();

    delete m_VulkanContext;
    printf("VulkanRenderer::Destroy\n");
}

void VulkanRenderer::Initialize(const char* applicationName) {
    printf("VulkanRenderer::Initialize\n");
    m_VulkanContext = new VulkanContext();

    m_VulkanContext->Initialize(applicationName);
}

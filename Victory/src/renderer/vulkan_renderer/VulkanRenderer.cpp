#include "VulkanRenderer.h"

#include "VulkanTypes.h"

VulkanRenderer::~VulkanRenderer() {
    Destroy();
    printf("VulkanRenderer::~\n");
}

void VulkanRenderer::Resize()
{
    printf("VulkanRenderer::Resize\n");
}

void VulkanRenderer::BeginFrame() {
    printf("VulkanRenderer::BeginFrame\n");
}

void VulkanRenderer::EndFrame() {
    printf("VulkanRenderer::EndFrame\n");
}

void VulkanRenderer::Destroy() {
    printf("VulkanRenderer::Destroy\n");

    m_VulkanContext.Cleanup();
}







void VulkanRenderer::Initialize(const char* applicationName) {
    printf("VulkanRenderer::Initialize\n");

    m_VulkanContext.Initialize(applicationName);
}

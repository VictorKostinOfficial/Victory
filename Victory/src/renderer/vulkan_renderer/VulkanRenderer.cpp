#include "VulkanRenderer.h"
#include "stdio.h"

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

void VulkanRenderer::Initialize() {
    printf("VulkanRenderer::Initialize\n");
}

void VulkanRenderer::Destroy() {
    printf("VulkanRenderer::Destroy\n");
}

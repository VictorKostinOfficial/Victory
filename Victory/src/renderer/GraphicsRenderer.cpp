#include "GraphicsRenderer.h"

#include "vulkan_renderer/VulkanRenderer.h"

GraphicsRenderer* GraphicsRenderer::s_Context = nullptr;

GraphicsRenderer *GraphicsRenderer::CreateInstance()
{
    if (!s_Context) {
        s_Context = VulkanRenderer::CreateInstance();
        // TODO: Give a choise between few Context (OpenGl, Metal, etc.)
    }
    return s_Context;
}

void GraphicsRenderer::DestroyInstance()
{
    delete s_Context;
}

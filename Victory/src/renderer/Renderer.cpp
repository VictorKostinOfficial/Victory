#include "Renderer.h"

#include <stdexcept>

#include "vulkan_renderer/VulkanRenderer.h"

Renderer* Renderer::s_Instance{ nullptr };

Renderer *Renderer::CreateRenderer(const char *applicationName)
{
    if (s_Instance) {
        throw std::runtime_error("Renderer already exists");
    }
    s_Instance = new VulkanRenderer();
    s_Instance->Initialize(applicationName);
    return s_Instance;
}
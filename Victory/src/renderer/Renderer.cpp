#include "Renderer.h"

#include <stdexcept>

#include "vulkan_renderer/VulkanRenderer.h"

Renderer* Renderer::s_Instance{ nullptr };

Renderer* Renderer::CreateRenderer() {
    if (s_Instance) {
        throw std::runtime_error("Renderer already exists");
    }
    s_Instance = VulkanRenderer::CreateRenderer();
    return s_Instance;
}

void Renderer::CleanupRenderer() {
    delete s_Instance;
}


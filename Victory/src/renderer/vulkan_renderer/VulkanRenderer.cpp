#include "VulkanRenderer.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

 void CollectLayers(std::vector<const char*>& layers_) {
 #ifdef _DEBUG
     layers_.emplace_back("VK_LAYER_KHRONOS_validation");
 #endif
 }

 void CollectExtensions(std::vector<const char*>& extensions_) {
 #ifdef _DEBUG
     extensions_.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
 #endif

     uint32_t glfwExtensionCount = 0;
     const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
     extensions_.reserve(glfwExtensionCount + 1);
     for (uint32_t i{0}; i < glfwExtensionCount; ++i) {
         extensions_.emplace_back(glfwExtensions[i]);
     }
 }

void CreateInstance(VulkanContext& vulkanContext_) {

    uint32_t version = VK_API_VERSION_1_1;
    vk::enumerateInstanceVersion(&version);

    vk::ApplicationInfo applicationInfo{
        "Victory Application",
        VK_MAKE_VERSION(1,0,0),
        "Victory Engine",
        VK_MAKE_VERSION(1,0,0),
        VK_API_VERSION_1_0
    };

    std::vector<const char*> layers;
    CollectLayers(layers);

    std::vector<const char*> extensions;
    CollectExtensions(extensions);

    vk::InstanceCreateInfo instanceCI{
        {},
        &applicationInfo,
        layers,
        extensions
    };

    vulkanContext_.Instance = vk::createInstance(instanceCI);
}

void VulkanRenderer::Initialize() {
    CreateInstance();
}

void VulkanRenderer::Resize() {

}

bool VulkanRenderer::BeginFrame() {

    return true;
}

bool VulkanRenderer::EndFrame() {

    return true;
}

void VulkanRenderer::Cleanup() {
}

GraphicsRenderer *VulkanRenderer::CreateInstance()
{
    return new VulkanRenderer();
}

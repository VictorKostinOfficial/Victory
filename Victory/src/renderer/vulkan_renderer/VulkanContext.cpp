#include "VulkanContext.h"
#include "stdio.h"

extern const char ** glfwGetRequiredInstanceExtensions(uint32_t* count_);

void VulkanContext::Initialize(const char* applicationName_) {
    vk::ApplicationInfo applicationInfo{
        applicationName_,
        VK_MAKE_VERSION(1,0,0),
        "Victory Engine",
        VK_MAKE_VERSION(1,0,0),
        VK_API_VERSION_1_1
    };

    std::vector<const char*> layers;
    std::vector<const char*> extensions;

    vk::InstanceCreateInfo instanceCI{{}, 
        &applicationInfo, 
        layers, 
        extensions
    };

    printf("Instance Created\n");
    m_Instance = vk::createInstance(instanceCI);
}

void VulkanContext::Cleanup() {
    printf("Instance Destroyed\n");
    m_Instance.destroy();
}

void VulkanContext::CollectLayers(std::vector<const char*>& layers_) {
#ifdef _DEBUG
    printf("_DEBUG\n");
    layers_.emplace_back("VK_LAYER_KHRONOS_validation");
#endif
}

void VulkanContext::CollectExtensions(std::vector<const char*>& extensions_) {
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
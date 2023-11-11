#include "VulkanContext.h"

#include <GLFW/glfw3.h>

#include <stdio.h>

void VulkanContext::Initialize(const char* applicationName_) {
    CreateInstance(applicationName_);
#ifndef NDEBUG
    RegisterDebugUtilsMessenger();
#endif // NDEBUG
    PickPhysicalDevice();
    CreateLogicalDevice();
}

void VulkanContext::Cleanup() {
    m_Device.destroy();
#ifndef NDEBUG
    vk::DispatchLoaderDynamic dynamicLoader(m_Instance, vkGetInstanceProcAddr);
    m_Instance.destroyDebugUtilsMessengerEXT(m_DebugMessenger, nullptr, dynamicLoader);
#endif // NDEBUG
    m_Instance.destroy();
    printf("VulkanInstance::Cleanup\n");
}

vk::Instance VulkanContext::GetInstance() const{
    return m_Instance;
}

vk::Device VulkanContext::GetDevice() const {
    return m_Device;
}

void VulkanContext::CreateInstance(const char* applicationName_) {
    uint32_t version = VK_API_VERSION_1_1;
    if (vk::enumerateInstanceVersion(&version) != vk::Result::eSuccess) {
        throw std::runtime_error("Vulkan version is not supported");
    }

    vk::ApplicationInfo applicationInfo{
        applicationName_,
        VK_MAKE_VERSION(1,0,0),
        "Victory Engine",
        VK_MAKE_VERSION(1,0,0),
        VK_API_VERSION_1_1
    };

    std::vector<const char*> layers;
    CollectLayers(layers);

    std::vector<const char*> extensions;
    CollectExtensions(extensions);

    vk::InstanceCreateInfo instanceCI{{}, 
        &applicationInfo, 
        layers, 
        extensions
    };

    printf("VulkanInstance::Initialize\n");
    m_Instance = vk::createInstance(instanceCI);
}

void VulkanContext::CollectLayers(std::vector<const char *> &layers_) {
#ifndef NDEBUG
    layers_.emplace_back("VK_LAYER_KHRONOS_validation");
#endif // NDEBUG
}

void VulkanContext::CollectExtensions(std::vector<const char*>& extensions_) {
#ifndef NDEBUG
    extensions_.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif // NDEBUG

    // TODO: Remove glfwInit
    glfwInit();
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    extensions_.reserve(glfwExtensionCount + 1);
    for (uint32_t i{0}; i < glfwExtensionCount; ++i) {
        extensions_.emplace_back(glfwExtensions[i]);
    }
}

#ifndef NDEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback( 
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    printf("\nValidation layer: %s", pCallbackData->pMessage);
    return VK_FALSE;
}

void VulkanContext::RegisterDebugUtilsMessenger()  {
    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
    debugCreateInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    debugCreateInfo.messageType = //vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
    debugCreateInfo.pfnUserCallback = DebugReportCallback;
    
    vk::DispatchLoaderDynamic dynamicLoader(m_Instance, vkGetInstanceProcAddr);
    m_DebugMessenger = m_Instance.createDebugUtilsMessengerEXT(debugCreateInfo, 
        nullptr, dynamicLoader);
}
#endif // NDEBUG

void VulkanContext::PickPhysicalDevice() {
    std::vector<vk::PhysicalDevice> physicalDevices = m_Instance.enumeratePhysicalDevices();
    for (auto&& phDevice : physicalDevices) {
        const vk::PhysicalDeviceProperties properties = phDevice.getProperties();
        const vk::PhysicalDeviceFeatures features = phDevice.getFeatures();

        bool discrete = properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;

#ifndef NDEBUG
        if (!discrete) {
            discrete = properties.deviceType == vk::PhysicalDeviceType::eCpu;
        }
#endif // NDEBUG

        if (!discrete) {
            continue;
        }

        if (PickQueueIndecies(phDevice)) {
            m_PhysicalDevice = phDevice;
            break;
        }
    }

    if (!m_PhysicalDevice) {
        throw std::runtime_error("Suitable GPU was not found");
    }
}

bool VulkanContext::PickQueueIndecies(vk::PhysicalDevice& phDevice_) {
    const std::vector<vk::QueueFamilyProperties> queueFamilyProperties =
        phDevice_.getQueueFamilyProperties();
    
    uint8_t min_score{UINT8_MAX};
    for(uint32_t i{0}; auto&& property : queueFamilyProperties) {
        uint8_t score{0};

        // Graphics queue
        if (m_GraphicsQueueIndex == UINT32_MAX && property.queueFlags & vk::QueueFlagBits::eGraphics) {
            m_GraphicsQueueIndex = i;
            ++score;

            // TODO: Present queue, need surface
            m_PresentQueueIndex = m_GraphicsQueueIndex;
        }

        // Compute queue
        if (property.queueFlags & vk::QueueFlagBits::eCompute) {
            m_ComputeQueueIndex = i;
            ++score;
        }

        // Transfer queue
        if (min_score > score && property.queueFlags & vk::QueueFlagBits::eTransfer) {
            min_score = score;
            m_TransferQueueIndex = i;
        }
    }

    if (m_GraphicsQueueIndex == UINT32_MAX) {
        // TODO: Complex check, now just check graphics Queue
        return false;
    }

    printf("\nGraphics Queue Index: %u", m_GraphicsQueueIndex);
    printf("\nPresent Queue Index: %u", m_PresentQueueIndex);
    printf("\nCompute Queue Index: %u", m_ComputeQueueIndex);
    printf("\nTransfer Queue Index: %u\n", m_TransferQueueIndex);

    return true;
}

void VulkanContext::CreateLogicalDevice() {
    std::vector<uint32_t> queueIndecies;
    queueIndecies.reserve(4);
    
    queueIndecies.emplace_back(m_GraphicsQueueIndex);
    if (m_GraphicsQueueIndex != m_PresentQueueIndex) {
        queueIndecies.emplace_back(m_PresentQueueIndex);
    }
    if (m_GraphicsQueueIndex != m_ComputeQueueIndex) {
        queueIndecies.emplace_back(m_ComputeQueueIndex);
    }
    if (m_GraphicsQueueIndex != m_TransferQueueIndex) {
        queueIndecies.emplace_back(m_TransferQueueIndex);
    }

    // TODO: if presentation and graphics is one queue index
    //       create 2 queue with queue priorities 0.9 and 1
    std::vector<vk::DeviceQueueCreateInfo> deviceQueueCIs;
    deviceQueueCIs.reserve(queueIndecies.size());
    std::vector<float> queuePriorities{ 0.9f, 1.f };
    for (size_t i{0}; float queueIndex : queueIndecies) {
        auto&& deviceQueueCI = 
            vk::DeviceQueueCreateInfo({}, queueIndex, 1, &queuePriorities[1]);
        deviceQueueCIs.emplace_back(deviceQueueCI);
    }
 
    std::vector<const char*> layers{};
    std::vector<const char*> extensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    vk::DeviceCreateInfo deviceCI{{},
        deviceQueueCIs,
        layers,
        extensions
    };
    m_Device = m_PhysicalDevice.createDevice(deviceCI);
}

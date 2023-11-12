#include "VulkanContext.h"

#include <GLFW/glfw3.h>

#include <stdio.h>
#include <set>

bool VulkanContext::CreateInstance(const char* applicationName_) {
    uint32_t version = VK_API_VERSION_1_1;
    if (vk::enumerateInstanceVersion(&version) != vk::Result::eSuccess) {
        printf("\nVulkan version is not supported");
        return false;
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

    m_Instance = vk::createInstance(instanceCI);

    printf("\nVulkanInstance::Initialize");
    return true;
}

bool VulkanContext::PickPhysicalDevice(vk::SurfaceKHR surface_) {
    std::vector<vk::PhysicalDevice> physicalDevices = m_Instance.enumeratePhysicalDevices();

    if (physicalDevices.size() == 0) {
        printf("\nFailed to find GPUs with Vulkan support");
        return false;
    }
    
    uint32_t rateScore{0};
    for (auto&& phDevice : physicalDevices) {
        const uint32_t score = RateDeviceSuitability(phDevice);
        if (rateScore < score) {
            rateScore = score;
            m_PhysicalDevice = phDevice;
        }
    }

    if (!m_PhysicalDevice) {
        printf("\nSuitable GPU was not found");
        return false;
    }

    if (!PickQueueIndecies(m_PhysicalDevice, surface_)) {
        printf("\nGraphics Family Queue was not found");
        return false;
    }

    return true;
}

bool VulkanContext::CreateLogicalDevice() {
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
    std::vector<float> queuePriorities{ 1.f, 0.9f };
    for (size_t i{0}; float queueIndex : queueIndecies) {
        vk::DeviceQueueCreateInfo&& deviceQueueCI = vk::DeviceQueueCreateInfo({}, 
            queueIndex, 
            1, 
            queueIndex == 0 ? &queuePriorities[0] : &queuePriorities[1]
        );
        deviceQueueCIs.emplace_back(deviceQueueCI);
    }
 
    std::vector<const char*> layers{};
    vk::PhysicalDeviceFeatures features{};

    vk::DeviceCreateInfo deviceCI{{},
        deviceQueueCIs,
        layers,
        m_DeviceExtensions,
        &features
    };

    m_Device = m_PhysicalDevice.createDevice(deviceCI);
    return true;
}

void VulkanContext::Cleanup() {
    m_Device.destroy();
#ifndef NDEBUG
    vk::DispatchLoaderDynamic dynamicLoader(m_Instance, vkGetInstanceProcAddr);
    m_Instance.destroyDebugUtilsMessengerEXT(m_DebugMessenger, nullptr, dynamicLoader);
#endif // NDEBUG
    m_Instance.destroy();
    printf("\nVulkanInstance::Cleanup");
}

vk::Instance VulkanContext::GetInstance() {
    return m_Instance;
}

vk::Device VulkanContext::GetDevice() {
    return m_Device;
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

void VulkanContext::CollectLayers(std::vector<const char *> &layers_) {
#ifndef NDEBUG
    layers_.emplace_back("VK_LAYER_KHRONOS_validation");
#endif // NDEBUG
}

void VulkanContext::CollectExtensions(std::vector<const char*>& extensions_) {
#ifndef NDEBUG
    extensions_.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif // NDEBUG

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    extensions_.reserve(glfwExtensionCount + 1);
    for (uint32_t i{0}; i < glfwExtensionCount; ++i) {
        extensions_.emplace_back(glfwExtensions[i]);
    }
}

uint32_t VulkanContext::RateDeviceSuitability(vk::PhysicalDevice phDevice_) {
    const vk::PhysicalDeviceFeatures features = phDevice_.getFeatures();
    if (!features.geometryShader) {
        return 0;
    }

    // Check other features ...

    const vk::PhysicalDeviceProperties properties = phDevice_.getProperties();

    uint32_t score{0};
    bool suitable = properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
    score += 100 * suitable;

#ifndef NDEBUG
    suitable = properties.deviceType == vk::PhysicalDeviceType::eCpu;
    score += 10 * suitable;
#endif // NDEBUG

    // Check other properties ...

    const std::vector<vk::ExtensionProperties> availableExtensions = 
        phDevice_.enumerateDeviceExtensionProperties();
    std::set<std::string> requiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());

    for (auto&& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
        if (requiredExtensions.empty()) {
            break;;
        }
    }

    if (!requiredExtensions.empty()) {
        return 0;
    }
    
    return score;
}

bool VulkanContext::PickQueueIndecies(vk::PhysicalDevice phDevice_, vk::SurfaceKHR surface_) {
    const std::vector<vk::QueueFamilyProperties> queueFamilyProperties =
        phDevice_.getQueueFamilyProperties();
    
    uint8_t min_score{UINT8_MAX};
    for(uint32_t i{0}; auto&& property : queueFamilyProperties) {
        uint8_t score{0};

        // Graphics queue
        if (m_GraphicsQueueIndex == UINT32_MAX && property.queueFlags & vk::QueueFlagBits::eGraphics) {
            m_GraphicsQueueIndex = i;
            ++score;
        }

        // Present queu
        if (m_PresentQueueIndex == UINT32_MAX && phDevice_.getSurfaceSupportKHR(i, surface_)) {
            m_PresentQueueIndex = i;
            ++score;
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

    printf("\nGraphics Queue Index: %u", m_GraphicsQueueIndex);
    printf("\nPresent Queue Index: %u", m_PresentQueueIndex);
    printf("\nCompute Queue Index: %u", m_ComputeQueueIndex);
    printf("\nTransfer Queue Index: %u", m_TransferQueueIndex);

    return !(m_GraphicsQueueIndex == UINT32_MAX);
}

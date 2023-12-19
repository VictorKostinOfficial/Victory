#include <vulkan/vulkan.h>
#include <vector>

#include "VulkanContext.h"

#include <GLFW/glfw3.h>
#include <string>
#include <iostream>

bool VulkanContext::CreateInstance(const char* applicationName_)
{
    VkApplicationInfo applicationInfo{};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.apiVersion = VK_API_VERSION_1_1;
    applicationInfo.pApplicationName = applicationName_;
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.pEngineName = "Victory Engine";
    applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    std::vector<const char*> layers;
    CollectLayers(layers);

    std::vector<const char*> extensions;
    CollectExtensions(extensions);

    VkInstanceCreateInfo instanceCI{};
    instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCI.pApplicationInfo = &applicationInfo;
    instanceCI.enabledLayerCount = static_cast<uint32_t>(layers.size());
    instanceCI.ppEnabledLayerNames = layers.data();
    instanceCI.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instanceCI.ppEnabledExtensionNames = extensions.data();

    return vkCreateInstance(&instanceCI, nullptr, &m_Instance) == VK_SUCCESS;
}

bool VulkanContext::PickPhysicalDevice(VkSurfaceKHR surface_) {
    uint32_t physicalDevicesCount{0};
    vkEnumeratePhysicalDevices(m_Instance, &physicalDevicesCount, nullptr);

    if (physicalDevicesCount == 0) {
        printf("\nAny Physical Devices were not found!");
        return false;
    }

    std::vector<VkPhysicalDevice> physicalDevices(physicalDevicesCount);
    vkEnumeratePhysicalDevices(m_Instance, &physicalDevicesCount, physicalDevices.data());

    uint32_t rateScore{0};
    for (auto&& phDevice : physicalDevices) {
        const uint32_t score = RateDeviceSuitability(phDevice);
        if (rateScore < score) {
            rateScore = score;
            m_PhysicalDevice = phDevice;
        }
    }

    if (!m_PhysicalDevice) {
        printf("\nSuitable GPU was not found!");
        return false;
    }

    if (!PickQueueIndecies(m_PhysicalDevice, surface_)) {
        printf("\nGraphics Family Queue was not found");
        return false;
    }

    m_MSAASamples = GetMaxUsableSampleCount();
    return true;
}

bool VulkanContext::CreateLogicalDevice() {
    // I don't need compute and transfer queues right now
    std::set<uint32_t> uniqueIndecies{m_QueueIndexes.GetUniqueIndexes()};

    std::vector<float> queuePriorities{ 1.f };
    std::vector<VkDeviceQueueCreateInfo> deviceQueueCIs;
    deviceQueueCIs.reserve(uniqueIndecies.size());
    VkDeviceQueueCreateInfo deviceQueueCI{};
    deviceQueueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    for (auto&& index : uniqueIndecies) {
        deviceQueueCI.queueFamilyIndex = index;
        deviceQueueCI.queueCount = 1;
        deviceQueueCI.pQueuePriorities = queuePriorities.data();
        deviceQueueCIs.emplace_back(deviceQueueCI);
    }

    std::vector<const char*> layers{};
    VkPhysicalDeviceFeatures features{};
    features.samplerAnisotropy = VK_TRUE;
    features.sampleRateShading = VK_TRUE;

    VkDeviceCreateInfo deviceCI{};
    deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCI.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCIs.size());
    deviceCI.pQueueCreateInfos = deviceQueueCIs.data();
    deviceCI.enabledLayerCount = static_cast<uint32_t>(layers.size());
    deviceCI.ppEnabledLayerNames = layers.data();
    deviceCI.enabledExtensionCount = static_cast<uint32_t>(m_DeviceExtensions.size());
    deviceCI.ppEnabledExtensionNames = m_DeviceExtensions.data();
    deviceCI.pEnabledFeatures = &features;

    return vkCreateDevice(m_PhysicalDevice, &deviceCI, nullptr, &m_Device) == VK_SUCCESS;
}

#ifndef NDEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback( 
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    (void)messageSeverity;
    (void)messageType;
    (void)pUserData;
    std::string check = pCallbackData->pMessage;
    std::cout << check << std::endl;
    // printf("\nValidation layer: %s", check.c_str());
    return VK_FALSE;
}

bool VulkanContext::RegisterDebugUtilsMessenger()  {
    VkDebugUtilsMessengerCreateInfoEXT debugCI{};
    debugCI.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCI.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCI.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCI.pfnUserCallback = DebugReportCallback;


    auto&& func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
    if (!func) {
        return false;
    }

    func(m_Instance, &debugCI, nullptr, &m_DebugMessenger);
    return true;
}

void VulkanContext::CleanupDebugUtilsMessenger() {
    auto&& func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(m_Instance, m_DebugMessenger, nullptr);
    }
}
#endif

uint32_t VulkanContext::FindMemoryType(const uint32_t typeFilter_, const VkMemoryPropertyFlags flags_) const {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter_ & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & flags_) == flags_) {
            return i;
        }
    }

    return UINT32_MAX;
}

void VulkanContext::GetQueue(VkQueue& queue_, QueueIndex queueIndex_) {
    uint32_t index = m_QueueIndexes.GetQueueIndex(queueIndex_);
    vkGetDeviceQueue(m_Device, index, 0, &queue_);
}

void VulkanContext::CleanupLogicalDevice() {
    vkDestroyDevice(m_Device, nullptr);
}

void VulkanContext::CleanupInstance() {
    vkDestroyInstance(m_Instance, nullptr);
}

void VulkanContext::CleanupAll() {
#ifndef NDEBUG
    CleanupDebugUtilsMessenger();
#endif
    CleanupLogicalDevice();
    CleanupInstance();
}

//----------------------------------------------------------------------
//--------------------------------PRIVATE-------------------------------
//----------------------------------------------------------------------
void VulkanContext::CollectLayers(std::vector<const char*> &layers_) {
#ifndef NDEBUG
    layers_.emplace_back("VK_LAYER_KHRONOS_validation");
    layers_.emplace_back("VK_LAYER_LUNARG_monitor");
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

VkSampleCountFlagBits VulkanContext::GetMaxUsableSampleCount() {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

uint32_t VulkanContext::RateDeviceSuitability(VkPhysicalDevice phDevice_) {
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(phDevice_, &features);

    if (features.geometryShader != VK_TRUE || features.samplerAnisotropy != VK_TRUE) {
        return 0;
    }

    // Check other features ...

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(phDevice_, &properties);

    uint32_t score{0};
    bool suitable = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    score += 100 * suitable;

#ifndef NDEBUG
    suitable = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU;
    score += 10 * suitable;
#endif // NDEBUG

    // Check other properties ...

    uint32_t availableExtensionsCount{};
    vkEnumerateDeviceExtensionProperties(phDevice_, nullptr, &availableExtensionsCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(availableExtensionsCount);
    vkEnumerateDeviceExtensionProperties(phDevice_, nullptr, &availableExtensionsCount, availableExtensions.data());

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

bool VulkanContext::PickQueueIndecies(VkPhysicalDevice phDevice_, VkSurfaceKHR surface_) {
    uint32_t queueFamilyPropertiesCount{0};
    vkGetPhysicalDeviceQueueFamilyProperties(phDevice_, &queueFamilyPropertiesCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertiesCount);
    vkGetPhysicalDeviceQueueFamilyProperties(phDevice_, &queueFamilyPropertiesCount, queueFamilyProperties.data());
    
    uint8_t min_score{UINT8_MAX};
    for(uint32_t i{0}, n = static_cast<uint32_t>(queueFamilyProperties.size()); i < n; ++i) {
        uint8_t score{0};

        // Graphics queue
        if (m_QueueIndexes.m_GraphicsQueueIndex == UINT32_MAX && queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            m_QueueIndexes.m_GraphicsQueueIndex = i;
            ++score;
        }

        // Present queu
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(phDevice_, i, surface_, &presentSupport);
        if (m_QueueIndexes.m_PresentQueueIndex == UINT32_MAX && presentSupport == VK_TRUE) {
            m_QueueIndexes.m_PresentQueueIndex = i;
            ++score;
        }

        // Compute queue
        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            m_QueueIndexes.m_ComputeQueueIndex = i;
            ++score;
        }

        // Transfer queue
        if (min_score > score && queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            min_score = score;
            m_QueueIndexes.m_TransferQueueIndex = i;
        }
    }

    return !(m_QueueIndexes.m_GraphicsQueueIndex == UINT32_MAX);
}

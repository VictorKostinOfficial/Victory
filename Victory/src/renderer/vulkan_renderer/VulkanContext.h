#pragma once

#include <vulkan/vulkan_core.h>

#include "VulkanTypes.h"

class VulkanContext {
public:

    bool CreateInstance(const char* applicationName_);
    bool PickPhysicalDevice(VkSurfaceKHR surface_);
    bool CreateLogicalDevice();

#ifndef NDEBUG
    bool RegisterDebugUtilsMessenger();
    void CleanupDebugUtilsMessenger();
#endif

    void CleanupLogicalDevice();
    void CleanupInstance();
    void CleanupAll();

    inline VkInstance GetInstance() const {
        return m_Instance;
    }

    inline VkDevice GetDevice() const {
        return m_Device;
    }

    inline VkPhysicalDevice GetPhysicalDevice() {
        return m_PhysicalDevice;
    }

    VkQueue GetQueue(QueueIndex queue_);

    inline QueueIndexes& GetQueueIndexes() {
        return m_QueueIndexes;
    }

private:

    void CollectLayers(std::vector<const char*>& layers_);
    void CollectExtensions(std::vector<const char*>& extensions_);

    std::vector<const char*> m_DeviceExtensions {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME 
    };

    uint32_t RateDeviceSuitability(VkPhysicalDevice phDevice_);
    bool PickQueueIndecies(VkPhysicalDevice phDevice_, VkSurfaceKHR surface_);

private:

    VkInstance m_Instance{VK_NULL_HANDLE};
    VkPhysicalDevice m_PhysicalDevice{VK_NULL_HANDLE};
    VkDevice m_Device{VK_NULL_HANDLE};

    QueueIndexes m_QueueIndexes;

#ifndef NDEBUG
    VkDebugUtilsMessengerEXT m_DebugMessenger{VK_NULL_HANDLE};
#endif
};
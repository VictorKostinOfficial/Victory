#pragma once

#include "VulkanTypes.h"
#include <vector>

class VulkanContext {
public:

    bool CreateInstance(const char* applicationName_);
    bool PickPhysicalDevice(VkSurfaceKHR surface_);
    bool CreateLogicalDevice();

#ifndef NDEBUG
    bool RegisterDebugUtilsMessenger();
    void CleanupDebugUtilsMessenger();
#endif

    uint32_t FindMemoryType(const uint32_t typeFilter_, const VkMemoryPropertyFlags flags_) const;
    VkFormat FindDepthFormat();
    VkFormat FindSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling_, VkFormatFeatureFlags features_);

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

    void GetQueue(VkQueue& queue_, QueueIndex queueIndex_);

    // inline VkCommandPool GetCommandPool() const {
    //     return m_CommandPool;
    // }

    inline QueueIndexes& GetQueueIndexes(){
        return m_QueueIndexes;
    }

    inline const VkSampleCountFlagBits GetSampleCount() const {
        return m_MSAASamples;
    }

private:

    void CollectLayers(std::vector<const char*>& layers_);
    void CollectExtensions(std::vector<const char*>& extensions_);
    VkSampleCountFlagBits GetMaxUsableSampleCount();

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
    VkSampleCountFlagBits m_MSAASamples = VK_SAMPLE_COUNT_1_BIT;

#ifndef NDEBUG
    VkDebugUtilsMessengerEXT m_DebugMessenger{VK_NULL_HANDLE};
#endif
};
#pragma once

#include <vulkan/vulkan.hpp>

class VulkanContext {
public:

    bool CreateInstance(const char* applicationName_);

    bool PickPhysicalDevice(vk::SurfaceKHR surface_);

    bool CreateLogicalDevice();

    void Cleanup();

    vk::Instance GetInstance();
    vk::Device GetDevice();
    vk::PhysicalDevice GetPhysicalDevice();
    uint32_t GetQueueIndex(uint8_t type = 0);

#ifndef NDEBUG
    void RegisterDebugUtilsMessenger();
#endif // NDEBUG

private:

    void CollectLayers(std::vector<const char*>& layers_);
    void CollectExtensions(std::vector<const char*>& extensions_);

    uint32_t RateDeviceSuitability(vk::PhysicalDevice phDevice_);
    bool PickQueueIndecies(vk::PhysicalDevice phDevice_, vk::SurfaceKHR surface_);

    vk::Queue GetGraphicsQueue();
    vk::Queue GetPresentQueue();

private:

    vk::Instance m_Instance{VK_NULL_HANDLE};
    vk::PhysicalDevice m_PhysicalDevice{VK_NULL_HANDLE};
    vk::Device m_Device{VK_NULL_HANDLE};

    std::vector<const char*> m_DeviceExtensions {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME 
    };

    uint32_t m_GraphicsQueueIndex{UINT32_MAX};
    uint32_t m_PresentQueueIndex{UINT32_MAX};
    uint32_t m_ComputeQueueIndex{UINT32_MAX};
    uint32_t m_TransferQueueIndex{UINT32_MAX};

    vk::Queue m_GraphicsQueue{VK_NULL_HANDLE};
    vk::Queue m_PresentQueue{VK_NULL_HANDLE};

#ifndef NDEBUG
    vk::DebugUtilsMessengerEXT m_DebugMessenger{VK_NULL_HANDLE};
#endif // NDEBUG
};
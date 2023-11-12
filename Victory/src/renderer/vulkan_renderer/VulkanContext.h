#pragma once

#include <vulkan/vulkan.hpp>

#include <optional>

class GLFWwindow;

class VulkanContext {
public:

    VulkanContext() = default;
    ~VulkanContext() = default;

    bool Initialize(const char* applicationName_);
    void Cleanup();

    vk::Instance GetInstance() const;
    vk::Device GetDevice() const;

private:

    void CreateInstance(const char* applicationName_);
    void CollectLayers(std::vector<const char*>& layers_);
    void CollectExtensions(std::vector<const char*>& extensions_);

#ifndef NDEBUG
    void RegisterDebugUtilsMessenger();
#endif // NDEBUG

    bool CreateSurface();
    bool PickPhysicalDevice();
    uint32_t RateDeviceSuitability(vk::PhysicalDevice& phDevice_);
    bool PickQueueIndecies(vk::PhysicalDevice& phDevice_);
    bool CreateLogicalDevice();

private:

    vk::Instance m_Instance{VK_NULL_HANDLE};
    vk::PhysicalDevice m_PhysicalDevice{VK_NULL_HANDLE};
    vk::Device m_Device{VK_NULL_HANDLE};
    vk::SurfaceKHR m_Surface{VK_NULL_HANDLE};

    std::vector<const char*> m_DeviceExtensions {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME 
    };

    uint32_t m_GraphicsQueueIndex{UINT32_MAX};
    uint32_t m_PresentQueueIndex{UINT32_MAX};
    uint32_t m_ComputeQueueIndex{UINT32_MAX};
    uint32_t m_TransferQueueIndex{UINT32_MAX};

    GLFWwindow* m_Window;

#ifndef NDEBUG
    vk::DebugUtilsMessengerEXT m_DebugMessenger{VK_NULL_HANDLE};
#endif // NDEBUG
};
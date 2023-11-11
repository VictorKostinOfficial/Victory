#pragma once

#include <vulkan/vulkan.hpp>

class VulkanContext {
public:

    VulkanContext() = default;
    ~VulkanContext() = default;

    void Initialize(const char* applicationName_);
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

    void PickPhysicalDevice();
    bool PickQueueIndecies(vk::PhysicalDevice& phDevice_);
    void CreateLogicalDevice();

private:

    vk::Instance m_Instance;
    uint32_t m_GraphicsQueueIndex{UINT32_MAX};
    uint32_t m_PresentQueueIndex{UINT32_MAX};
    uint32_t m_ComputeQueueIndex{UINT32_MAX};
    uint32_t m_TransferQueueIndex{UINT32_MAX};
    vk::PhysicalDevice m_PhysicalDevice;
    vk::Device m_Device;

#ifndef NDEBUG
    vk::DebugUtilsMessengerEXT m_DebugMessenger{ VK_NULL_HANDLE };
#endif // NDEBUG
};
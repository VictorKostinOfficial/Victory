#pragma once

#include "VulkanTypes.h"

class VulkanContext {
public:

    VulkanContext() = default;
    ~VulkanContext() = default;

    void Initialize(const char* applicationName_);
    void Cleanup();

    vk::Instance GetInstance() const;
    vk::Device GetDevice() const;

private:

    void CreateInstance();
    void CollectLayers(std::vector<const char*>& layers_);
    void CollectExtensions(std::vector<const char*>& extensions_);

    void PickPhysicalDevice();
    void CreateLogicalDevice();

    vk::Instance m_Instance;
    vk::PhysicalDevice m_PhysicalDevice;
    vk::Device m_Device;
};
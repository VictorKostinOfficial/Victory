#pragma once

#include <vulkan/vulkan.hpp>

struct PhysicsDeviceQueueIndices {
    uint32_t GraphicsQueueIndex;
    uint32_t PresentQueueIndex;
    uint32_t ComputeQueueIndex;
    uint32_t TransferQueueIndex;
};

struct VulkanContext {
    vk::Instance Instance;
#ifdef _DEBUG
    vk::DebugUtilsMessengerEXT DebugMessenger;
#endif
    PhysicsDeviceQueueIndices QueueIndices;
    vk::Device Device;
};
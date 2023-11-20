#pragma once

#include <vulkan/vulkan_core.h>
#include "VulkanContext.h"

class VertexBuffer
{
public:

    bool CreateVertexBuffer(VkDevice device_);
    bool MemoryAllocation(VulkanContext& context_);

    void FreeMemory(VkDevice device_);
    void CleanupVertexBuffer(VkDevice device_);
    void CleanupAll(VkDevice device_);

    VkBuffer GetBuffer() const;
    uint32_t GetVertexSize() const;

private:

    uint32_t FindMemoryType(VkPhysicalDevice phDevice_, uint32_t typeFilter_, VkMemoryPropertyFlags flags_);

private:

    VkBuffer m_VertexBuffer{VK_NULL_HANDLE};
    VkDeviceMemory m_VertexBufferMemory{VK_NULL_HANDLE};
};

#pragma once

#include <vulkan/vulkan_core.h>

class VulkanContext;
class VulkanFrameBuffer;

class VertexBuffer
{
public:
    VertexBuffer(VulkanContext* context_, VulkanFrameBuffer* frameBuffer_);

    bool CreateVertexBuffer();
    bool CreateIndexBuffer();

    void FreeMemory();
    void CleanupVertexBuffer();
    void CleanupAll();

    inline VkBuffer GetVertexBuffer() const {
        return m_VertexBuffer;
    }

    inline VkBuffer GetIndexBuffer() const {
        return m_IndexBuffer;
    }

    uint32_t GetIndicesCount() const;

private:

    bool CreateBuffer(VkDeviceSize bufferSize_, VkBufferUsageFlags usage_, 
        VkMemoryPropertyFlags properties, VkBuffer& buffer_, VkDeviceMemory& bufferMemory_);
    uint32_t FindMemoryType(uint32_t typeFilter_, VkMemoryPropertyFlags flags_);
    void CopyBuffer(VkBuffer srcBuffer_, VkBuffer dstBuffer_, VkDeviceSize size_);

private:

    VulkanContext* m_Context;
    VulkanFrameBuffer* m_FrameBuffer;

    VkBuffer m_VertexBuffer{VK_NULL_HANDLE};
    VkDeviceMemory m_VertexBufferMemory{VK_NULL_HANDLE};
    VkBuffer m_IndexBuffer{VK_NULL_HANDLE};
    VkDeviceMemory m_IndexBufferMemory{VK_NULL_HANDLE};
};

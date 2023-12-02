#pragma once

#include "../VertexData.h"

class VulkanContext;
class VulkanFrameBuffer;

struct CreateBufferSettings {
    VkDeviceSize Size;
    VkBufferUsageFlags Usage;
    VkMemoryPropertyFlags Properties;
};

class VulkanModel {
public:

    VulkanModel(VulkanContext* context_
    , VulkanFrameBuffer* frameBuffer_);

    bool LoadModel(std::string&& path_);
    bool CreateVertexBuffer();
    bool CreateIndexBuffer();

    void CleanupBuffer(VkBuffer& buffer_);
    void FreeBufferMemory(VkDeviceMemory& memory_);
    void CleanupAll();

    inline const VkBuffer GetVertexBuffer() const {
        return m_VertexBuffer;
    }

    inline const VkBuffer GetIndexBuffer() const {
        return m_IndexBuffer;
    }

    inline const uint32_t GetIndexCount() const {
        return static_cast<uint32_t>(m_Indices.size());
    }

private:

    void CopyBuffer(VkBuffer srcBuffer_, VkBuffer dstBuffer_, VkDeviceSize size_);

private:

    std::vector<VertexData> m_Vertices;
    std::vector<uint16_t> m_Indices;

    VulkanContext* m_Context;
    VulkanFrameBuffer* m_FrameBuffer;

    VkBuffer m_VertexBuffer{VK_NULL_HANDLE};
    VkDeviceMemory m_VertexBufferMemory{VK_NULL_HANDLE};
    VkBuffer m_IndexBuffer{VK_NULL_HANDLE};
    VkDeviceMemory m_IndexBufferMemory{VK_NULL_HANDLE};
};

bool BindBuffer(const VulkanContext* context_, const CreateBufferSettings& bufferSettings_, 
    VkBuffer& buffer_, VkDeviceMemory& bufferMemory_);

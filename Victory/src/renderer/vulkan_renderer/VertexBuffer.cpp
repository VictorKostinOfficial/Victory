#include "VertexBuffer.h"
#include <vector>
#include <cstring>
#include "../VertexData.h"

#include "VulkanFrameBuffer.h"

const std::vector<VertexData> vertices = {
    {{0.0f, -0.5f, 1}, {1.0f, 1.0f, 1.0f}},
    {{0.5f, 0.5f, 1}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f, 1}, {0.0f, 0.0f, 1.0f}}
};

VertexBuffer::VertexBuffer(VulkanContext *context_, VulkanFrameBuffer *frameBuffer_) 
    : m_Context{context_}
    , m_FrameBuffer{frameBuffer_} {
}

bool VertexBuffer::CreateVertexBuffer()
{
    VkDeviceSize bufferSize = static_cast<uint64_t>(sizeof(vertices[0]) * vertices.size());

    VkBuffer stagingBuffer{VK_NULL_HANDLE};
    VkDeviceMemory stagingBufferMemory{VK_NULL_HANDLE};

    if (CreateVertexBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory) != true) {
        return false;
    }

    void* data;
    if (vkMapMemory(m_Context->GetDevice(), stagingBufferMemory, 0, sizeof(vertices[0]) * vertices.size(), 0, &data) != VK_SUCCESS) {
        return false;
    }
        memcpy(data, vertices.data(), bufferSize);
    vkUnmapMemory(m_Context->GetDevice(), stagingBufferMemory);

    if (CreateVertexBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VertexBuffer, m_VertexBufferMemory) != true) {
            return false;
    };
    
    CopyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);

    vkDestroyBuffer(m_Context->GetDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_Context->GetDevice(), stagingBufferMemory, nullptr);
    return true;
}

bool VertexBuffer::CreateVertexBuffer(VkDeviceSize size_, VkBufferUsageFlags usage_, VkMemoryPropertyFlags properties_, VkBuffer& buffer_, VkDeviceMemory& bufferMemory_) {
    VkBufferCreateInfo bufferCI{};
    bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCI.size = size_;
    bufferCI.usage = usage_;
    bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_Context->GetDevice(), &bufferCI, nullptr, &buffer_) != VK_SUCCESS) {
        return false;
    }

    VkMemoryRequirements memRequirements{};
    vkGetBufferMemoryRequirements(m_Context->GetDevice(), buffer_, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties_);
    if (allocInfo.memoryTypeIndex == UINT32_MAX) {
        return false;
    }

    if (vkAllocateMemory(m_Context->GetDevice(), &allocInfo, nullptr, &bufferMemory_) != VK_SUCCESS) {
        return false;
    }

    return vkBindBufferMemory(m_Context->GetDevice(), buffer_, bufferMemory_, 0) == VK_SUCCESS;
}

void VertexBuffer::FreeMemory() {
    vkFreeMemory(m_Context->GetDevice(), m_VertexBufferMemory, nullptr);
}

void VertexBuffer::CleanupVertexBuffer() {
    vkDestroyBuffer(m_Context->GetDevice(), m_VertexBuffer, nullptr);
}

void VertexBuffer::CleanupAll() {
    CleanupVertexBuffer();
    FreeMemory();
}

VkBuffer VertexBuffer::GetBuffer() const {
    return m_VertexBuffer;
}

uint32_t VertexBuffer::GetVertexSize() const {
    return static_cast<uint32_t>(vertices.size());
}

uint32_t VertexBuffer::FindMemoryType(uint32_t typeFilter_, VkMemoryPropertyFlags flags_) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_Context->GetPhysicalDevice(), &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter_ & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & flags_) == flags_) {
            return i;
        }
    }

    return UINT32_MAX;
}

void VertexBuffer::CopyBuffer(VkBuffer srcBuffer_, VkBuffer dstBuffer_, VkDeviceSize size_) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_FrameBuffer->GetCommandPool();
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_Context->GetDevice(), &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    {
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size_;

        vkCmdCopyBuffer(commandBuffer, srcBuffer_, dstBuffer_, 1, &copyRegion);
    }
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    // TODO: submit via transfer queue, need Transfer Command pool
    // https://vulkan-tutorial.com/Vertex_buffers/Staging_buffer

    VkQueue queue = m_Context->GetQueue(QueueIndex::eGraphics);
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(m_Context->GetDevice(), m_FrameBuffer->GetCommandPool(), 1, &commandBuffer);
}

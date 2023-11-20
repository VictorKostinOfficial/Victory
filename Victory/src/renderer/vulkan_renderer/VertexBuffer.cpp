#include "VertexBuffer.h"
#include <vector>
#include "../VertexData.h"

const std::vector<VertexData> vertices = {
    {{0.0f, -0.5f, 1}, {1.0f, 1.0f, 1.0f}},
    {{0.5f, 0.5f, 1}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f, 1}, {0.0f, 0.0f, 1.0f}}
};


bool VertexBuffer::CreateVertexBuffer(VkDevice device_) {
    VkBufferCreateInfo bufferCI{};
    bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCI.size = sizeof(vertices[0]) * vertices.size();
    bufferCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    return vkCreateBuffer(device_, &bufferCI, nullptr, &m_VertexBuffer) == VK_SUCCESS;
}

bool VertexBuffer::MemoryAllocation(VulkanContext& context_) {
    VkDevice device = context_.GetDevice();
    VkMemoryRequirements memRequirements{};
    vkGetBufferMemoryRequirements(device, m_VertexBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(context_.GetPhysicalDevice(), memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (allocInfo.memoryTypeIndex == UINT32_MAX) {
        return false;
    }

    if (vkAllocateMemory(device, &allocInfo, nullptr, &m_VertexBufferMemory) != VK_SUCCESS) {
        return false;
    }

    if (vkBindBufferMemory(device, m_VertexBuffer, m_VertexBufferMemory, 0) != VK_SUCCESS) {
        return false;
    }

    void* data;
    if (vkMapMemory(device, m_VertexBufferMemory, 0, sizeof(vertices[0]) * vertices.size(), 0, &data) != VK_SUCCESS) {
        return false;
    }
        memcpy(data, vertices.data(), static_cast<size_t>(sizeof(vertices[0]) * vertices.size()));
    vkUnmapMemory(device, m_VertexBufferMemory);

    return true;
}

void VertexBuffer::FreeMemory(VkDevice device_) {
    vkFreeMemory(device_, m_VertexBufferMemory, nullptr);
}

void VertexBuffer::CleanupVertexBuffer(VkDevice device_) {
    vkDestroyBuffer(device_, m_VertexBuffer, nullptr);
}

void VertexBuffer::CleanupAll(VkDevice device_) {
    CleanupVertexBuffer(device_);
    FreeMemory(device_);
}

VkBuffer VertexBuffer::GetBuffer() const {
    return m_VertexBuffer;
}

uint32_t VertexBuffer::GetVertexSize() const {
    return static_cast<uint32_t>(vertices.size());
}

uint32_t VertexBuffer::FindMemoryType(VkPhysicalDevice phDevice_, uint32_t typeFilter_, VkMemoryPropertyFlags flags_) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(phDevice_, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter_ & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & flags_) == flags_) {
            return i;
        }
    }

    return UINT32_MAX;
}

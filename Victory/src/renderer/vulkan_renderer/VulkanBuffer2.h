#pragma once

class VulkanContext;

struct CreateBufferSettings {
    VkDeviceSize Size;
    VkBufferUsageFlags Usage;
    VkMemoryPropertyFlags Properties;
};

bool CreateBuffer(const VulkanContext* context_, const CreateBufferSettings& bufferSettings_, 
    VkBuffer& buffer_, VkDeviceMemory& bufferMemory_);

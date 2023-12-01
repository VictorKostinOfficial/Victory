#include <vulkan/vulkan.h>

#include "VulkanBuffer2.h"

#include "VulkanContext.h"

bool CreateBuffer(const VulkanContext* context_, const CreateBufferSettings &bufferSettings_, 
        VkBuffer &buffer_, VkDeviceMemory &bufferMemory_) {
    VkBufferCreateInfo bufferCI{};
    bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCI.size = bufferSettings_.Size;
    bufferCI.usage = bufferSettings_.Usage;
    bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(context_->GetDevice(), &bufferCI, nullptr, &buffer_) != VK_SUCCESS) {
        return false;
    }

    VkMemoryRequirements memRequirements{};
    vkGetBufferMemoryRequirements(context_->GetDevice(), buffer_, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = context_->FindMemoryType(memRequirements.memoryTypeBits, bufferSettings_.Properties);
    if (allocInfo.memoryTypeIndex == UINT32_MAX) {
        return false;
    }

    vkAllocateMemory(context_->GetDevice(), &allocInfo, nullptr, &bufferMemory_);

    return vkBindBufferMemory(context_->GetDevice(), buffer_, bufferMemory_, 0) == VK_SUCCESS;
}

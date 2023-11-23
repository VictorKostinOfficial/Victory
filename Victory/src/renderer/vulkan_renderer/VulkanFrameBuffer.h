#pragma once

#include <vulkan/vulkan_core.h>

#include "VulkanPipeline.h"
#include "VertexBuffer.h"

class VulkanFrameBuffer {
public:

    bool CreateFrameBuffers(VulkanContext& context_, VulkanSwapchain& swapchain_, VulkanPipeline& pipeline_);
    bool CreateCommandPool(VulkanContext& context_);
    bool CreateCommandBuffer(VkDevice device_, uint32_t commandBufferCount_);

    // TODO: move function to VulkanRenderer
    void RecordCommandBuffer(VulkanSwapchain& swapchain_, VulkanPipeline& pipeline_, uint32_t commandBufferIndex_, uint32_t imageIndex_, VertexBuffer& vertexBuffer_);

    void CleanupCommandPool(VkDevice device_);
    void CleanupFrameBuffers(VkDevice device_);
    void CleanupAll(VulkanContext& context_);

    VkCommandBuffer GetCommandBuffer(uint32_t commandBufferIndex) const;
    inline VkCommandPool GetCommandPool() {
        return m_CommandPool;
    }

private:

    std::vector<VkFramebuffer> m_FrameBuffers;
    VkCommandPool m_CommandPool{VK_NULL_HANDLE};
    std::vector<VkCommandBuffer> m_CommandBuffers;
};

#pragma once

#include <vulkan/vulkan_core.h>
#include <vector>

class VulkanContext;
class VulkanSwapchain;
class VulkanPipeline;
class VulkanBuffer;

class VulkanFrameBuffer {
public:

    VulkanFrameBuffer(VulkanContext* context_, VulkanSwapchain* swapchain_, 
        VulkanPipeline* pipeline_, VulkanBuffer* buffer_);
    void SetVulkanBuffer(VulkanBuffer* buffer_);

    bool CreateFrameBuffers();
    bool CreateCommandPool();
    bool CreateCommandBuffer(uint32_t commandBufferCount_);

    // TODO: move function to VulkanRenderer
    void RecordCommandBuffer(uint32_t commandBufferIndex_, uint32_t imageIndex_);
    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer_);

    void CleanupCommandPool();
    void CleanupFrameBuffers();
    void CleanupAll();

    inline VkCommandBuffer GetCommandBuffer(uint32_t commandBufferIndex) const {
        return m_CommandBuffers[commandBufferIndex];
    }

    inline VkCommandPool GetCommandPool() const {
        return m_CommandPool;
    }

private:

    VulkanContext* m_Context;
    VulkanSwapchain* m_Swapchain;
    VulkanPipeline* m_Pipeline;
    VulkanBuffer* m_Buffer;

    std::vector<VkFramebuffer> m_FrameBuffers;
    VkCommandPool m_CommandPool{VK_NULL_HANDLE};
    std::vector<VkCommandBuffer> m_CommandBuffers;
};

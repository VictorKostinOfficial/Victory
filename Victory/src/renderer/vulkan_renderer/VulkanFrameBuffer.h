#pragma once

class VulkanContext;
class VulkanSwapchain;
class VulkanPipeline;
class VulkanBuffer;
class VulkanImage;

class VulkanFrameBuffer {
public:

    VulkanFrameBuffer(VulkanContext* context_, VulkanSwapchain* swapchain_, 
        VulkanPipeline* pipeline_);

    bool CreateFrameBuffers(VkRenderPass pass_, bool isImGui_ = false);
    bool CreateCommandPool();
    bool CreateCommandBuffer(uint32_t commandBufferCount_);

    bool CreateDepthResources();
    bool CreateColorResources();

    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer_);

    void CleanupDepthResources();
    void CleanupColorResources();

    void CleanupCommandPool();
    void CleanupFrameBuffers();
    void CleanupAll();

    inline VkCommandBuffer GetCommandBuffer(const uint32_t index) const {
        return m_CommandBuffers[index];
    }

    inline VkCommandPool GetCommandPool() const {
        return m_CommandPool;
    }

    inline VkFramebuffer GetFrameBuffer(const uint32_t index) const {
        return m_FrameBuffers[index];
    }

private:

    VulkanContext* m_Context;
    VulkanSwapchain* m_Swapchain;
    VulkanPipeline* m_Pipeline;
    VulkanBuffer* m_Buffer;

    std::vector<VkFramebuffer> m_FrameBuffers;
    VkCommandPool m_CommandPool{VK_NULL_HANDLE};
    std::vector<VkCommandBuffer> m_CommandBuffers;

    VulkanImage* m_DepthImage;
    VulkanImage* m_ColorImage;
};

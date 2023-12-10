#pragma once

class VulkanContext;
class VulkanSwapchain;
class VulkanPipeline;
class VulkanBuffer;
class VulkanImage;

class VulkanFrameBuffer {
public:

    VulkanFrameBuffer(VulkanContext* context_);

    bool CreateFrameBuffers(VkRenderPass pass_, const VkExtent2D& extent_,
            const std::vector<VkImageView>& imageViews_, const bool isImGui_ = false);
    bool CreateCommandBuffer(uint32_t commandBufferCount_);

    bool CreateDepthResources(const VkFormat depthFormat_, const VkExtent2D& extent_);
    bool CreateColorResources(const VkFormat colorFormat_, const VkExtent2D& extent_);

    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer_);

    void CleanupDepthResources();
    void CleanupColorResources();

    void CleanupFrameBuffers();
    void CleanupAll();

    inline VkCommandBuffer GetCommandBuffer(const uint32_t index) const {
        return m_CommandBuffers[index];
    }

    inline VkFramebuffer GetFrameBuffer(const uint32_t index) const {
        return m_FrameBuffers[index];
    }

private:

    VulkanContext* m_Context;

    std::vector<VkFramebuffer> m_FrameBuffers;
    std::vector<VkCommandBuffer> m_CommandBuffers;

    VulkanImage* m_DepthImage;
    VulkanImage* m_ColorImage;
};

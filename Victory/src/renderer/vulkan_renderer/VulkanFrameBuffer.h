#pragma once

class VulkanContext;
class VulkanImage;
enum class QueueIndex;

class VulkanFrameBuffer {
public:

    VulkanFrameBuffer(VulkanContext* context_);
    bool CreateCommandPool(QueueIndex index_);

    bool CreateFrameBuffers(VkRenderPass pass_, const VkExtent2D& extent_,
            const std::vector<VulkanImage>& images_, const bool isImGui_ = false);
    bool CreateCommandBuffer(uint32_t commandBufferCount_);

    bool CreateDepthResources(const VkFormat depthFormat_, const VkExtent2D& extent_);
    bool CreateColorResources(const VkFormat colorFormat_, const VkExtent2D& extent_);

    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer_);

    void CleanupDepthResources();
    void CleanupColorResources();

    void CleanupFrameBuffers();
    void CleanupCommandPool();
    void CleanupAll(bool bClearResources = false);

    inline VkCommandBuffer GetCommandBuffer(const uint32_t index) const {
        return m_CommandBuffers[index];
    }

    inline VkFramebuffer GetFrameBuffer(const uint32_t index) const {
        return m_FrameBuffers[index];
    }

private:

    VulkanContext* m_Context;
    VkCommandPool m_CommandPool{VK_NULL_HANDLE};

    std::vector<VkFramebuffer> m_FrameBuffers;
    std::vector<VkCommandBuffer> m_CommandBuffers;

    VulkanImage* m_DepthImage;
    VulkanImage* m_ColorImage;
};

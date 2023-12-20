#pragma once

class VulkanContext;
class VulkanImage;
enum class QueueIndex;
struct CreateImageSettings;

class VulkanFrameBuffer {
public:

    VulkanFrameBuffer(VulkanContext* context_, VkRenderPass renderPass_);
    bool CreateCommandPool(QueueIndex index_);

    bool CreateFrameBuffers(const CreateImageSettings& settings_, uint32_t frameBufferCount_);
    bool CreateFrameBuffers(const CreateImageSettings& settings_, std::vector<VkImage>& images_);
    bool CreateCommandBuffer(uint32_t commandBufferCount_);

    void AddAttachment(const CreateImageSettings settings_);

    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer_);

    void CleanupFrameBuffers(bool cleanupAll_);
    void CleanupCommandPool();
    void CleanupAll(bool cleanupAll_ = false);

    inline VkCommandBuffer GetCommandBuffer(const uint32_t index) const {
        return m_CommandBuffers[index];
    }

    inline VkFramebuffer GetFrameBuffer(const uint32_t index) const {
        return m_FrameBuffers[index];
    }

    inline const std::vector<VulkanImage>& GetFrameImages() const {
        return m_FrameImages;
    }

private:

    VulkanContext* m_Context;
    VkRenderPass m_RenderPass;

    VkCommandPool m_CommandPool{VK_NULL_HANDLE};

    std::vector<VkFramebuffer> m_FrameBuffers;
    std::vector<VkCommandBuffer> m_CommandBuffers;

    std::vector<VulkanImage> m_FrameImages;
    std::vector<VulkanImage> m_AttachmentImages;
    VulkanImage* m_DepthImage;
    VulkanImage* m_ColorImage;
};

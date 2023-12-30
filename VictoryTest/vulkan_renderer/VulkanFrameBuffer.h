#pragma once

namespace Victory { 
    class VulkanDevice;
    class VulkanImage;

    enum class QueueIndex;

    class VulkanFrameBuffer {
    public:

        VulkanFrameBuffer(VulkanDevice* vulkanDevice_);

        void CreateCommandPool(QueueIndex index_);

        void CreateFrameBufferImages(const VkImageCreateInfo& imageCI, 
            const uint32_t frameBufferCount_);
        void AttachFrameBufferImages(std::vector<VkImage>& images_, VkFormat format_);

        void SetAttachments(const std::vector<VulkanImage>& imageViews_);

        void CreateFrameBuffers(VkRenderPass renderPass_, VkExtent2D extent_);

        void CreateCommandBuffers();

        // void AddAttachment(const VkImageCreateInfo& settings_);

        // VkCommandBuffer BeginSingleTimeCommands();
        // void EndSingleTimeCommands(VkCommandBuffer commandBuffer_);

        void CleanupAttachments();
        void CleanupFrameBuffers(bool cleanupAll_);
        void CleanupCommandPool();
        void CleanupAll();

        inline VkCommandBuffer GetCommandBuffer(const uint32_t index_) const {
            return m_CommandBuffers[index_];
        }

        inline VkFramebuffer GetFrameBuffer(const uint32_t index_) const {
            return m_FrameBuffers[index_];
        }

        inline const std::vector<VulkanImage>& GetFrameImages() const {
            return m_FrameImages;
        }

    private:

        VulkanDevice* m_VulkanDevice { nullptr };

        VkCommandPool m_CommandPool{VK_NULL_HANDLE};

        std::vector<VkFramebuffer> m_FrameBuffers;
        std::vector<VkCommandBuffer> m_CommandBuffers;

        std::vector<VulkanImage> m_FrameImages;
        std::vector<VulkanImage> m_AttachmentImages;
    };
}
#pragma once

namespace Victory 
{ 
    class VulkanDevice;
    class VulkanImage;

    enum class QueueIndex;

    class VulkanFrameBuffer 
    {
    public:

        VulkanFrameBuffer(VulkanDevice* vulkanDevice_);

        void CreateCommandPool(QueueIndex index_);

        void CreateFrameBufferImages(const VkImageCreateInfo& imageCI, 
            const uint32_t frameBufferCount_);
        void AttachFrameBufferImages(std::vector<VkImage>& images_, VkFormat format_);

        void SetAttachments(const std::vector<VulkanImage>& imageViews_);

        void CreateFrameBuffers(VkRenderPass renderPass_, VkExtent2D extent_);

        void CreateCommandBuffers();

        void CleanupAttachments();
        void CleanupCommandPool();
        void CleanupAll();

        inline VkCommandBuffer GetCommandBuffer(const uint32_t index_) const {
            return m_CommandBuffers[index_];
        }

        inline VkFramebuffer GetFrameBuffer(const uint32_t index_) const {
            return m_FrameBuffers[index_];
        }

        inline std::vector<VulkanImage>& GetFrameImages() {
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
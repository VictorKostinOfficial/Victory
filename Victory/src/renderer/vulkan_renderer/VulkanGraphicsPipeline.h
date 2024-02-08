#pragma once

namespace Victory 
{
    class VulkanGraphicsPipeline 
    {
    public:

        VulkanGraphicsPipeline() {};
        virtual ~VulkanGraphicsPipeline() {};

        virtual void InitResources(VkFormat format_ = VK_FORMAT_UNDEFINED, 
            VkExtent2D extent_ = {}, const uint32_t frameBuffersCount_ = 0) = 0;

        virtual VkCommandBuffer BeginFrame(const uint32_t currentFrame_) = 0;
        virtual void RecordBuffer(const uint32_t imageIndex_) = 0;
        virtual void EndFrame() = 0;

        virtual void RecreateResources() = 0;

        virtual const VkRenderPass GetRenderPass() const {
            return m_RenderPass;
        }

        virtual const VkPipeline GetPipeline() const {
            return m_Pipeline;
        }

    protected:

        VkImageCreateInfo m_FramesImageCI{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };

        VulkanDevice* m_VulkanDevice{ nullptr };
        VulkanSwapchain* m_VulkanSwapchain{ nullptr };

        VkPipelineCache m_PipelineCache{ VK_NULL_HANDLE };
        VkRenderPass m_RenderPass{ VK_NULL_HANDLE };
        VkPipeline m_Pipeline{ VK_NULL_HANDLE };

        VkPipelineLayout m_PipelineLayout{ VK_NULL_HANDLE };
        VkDescriptorSetLayout m_DescriptorSetLayout{ VK_NULL_HANDLE };

        VulkanFrameBuffer* m_FrameBuffer;
        VkCommandBuffer m_CurrentCommandBuffer;
    };
}
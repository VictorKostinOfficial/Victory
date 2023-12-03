#pragma once

class VulkanContext;
class VulkanFrameBuffer;
class VulkanSwapchain;
class VulkanPipeline;

class VulkanBuffer
{
public:
    VulkanBuffer(VulkanContext* context_, VulkanPipeline* pipeline_, 
        VulkanFrameBuffer* frameBuffer_, VulkanSwapchain* swapchain_);

    // TODO: Create UnifomBuffer class?
    // TODO: Create Descriptor class? 
    bool CreateUniformBuffers(uint32_t maxFrames_);
    bool CreateDescriptorPool(uint32_t maxFrames_);
    bool CreateDescriptorSets(uint32_t maxFrames_, const VkSampler& sampler_, const VkImageView& imageView_);

    void UpdateUniformBuffer(uint32_t imageIndex_);

    void FreeMemory();
    void CleanupVertexBuffer();
    void CleanupAll();

    inline const VkDescriptorSet& GetDescriptorSet(uint32_t imageIndex_) const {
        return m_DescriptorSets[imageIndex_];
    }

    inline const VkDescriptorPool& GetDescriptorPool() const {
        return m_DescriptorPool;
    }

private:

    bool HasStencilComponent(VkFormat format_);

private:

    VulkanContext* m_Context;
    VulkanPipeline* m_Pipeline;
    VulkanFrameBuffer* m_FrameBuffer;
    VulkanSwapchain* m_Swapchain;

    VkDescriptorPool m_DescriptorPool{VK_NULL_HANDLE};
    std::vector<VkDescriptorSet> m_DescriptorSets;

    std::vector<VkBuffer> m_UniformBuffers;
    std::vector<VkDeviceMemory> m_UniformBuffersMemory;
    std::vector<void*> m_UniformBuffersMapped;

};

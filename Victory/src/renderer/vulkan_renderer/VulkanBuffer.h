#pragma once

#include <vulkan/vulkan_core.h>
#include <vector>

class VulkanContext;
class VulkanFrameBuffer;
class VulkanSwapchain;
class VulkanPipeline;

class VulkanBuffer
{
public:
    VulkanBuffer(VulkanContext* context_, VulkanPipeline* pipeline_, 
        VulkanFrameBuffer* frameBuffer_, VulkanSwapchain* swapchain_);

    bool CreateDepthResources();

    bool CreateTextureImage();
    bool CreateTextureImageView();
    bool CreateTextureImageView(VkImage image_, VkFormat format_);
    bool CreateTextureSampler();

    bool CreateVertexBuffer();
    bool CreateIndexBuffer();
    bool CreateUniformBuffers(uint32_t maxFrames_);
    bool CreateDescriptorPool(uint32_t maxFrames_);
    bool CreateDescriptorSets(uint32_t maxFrames_);

    VkFormat FindDepthFormat();
    void UpdateUniformBuffer(uint32_t imageIndex_);

    void CleanupDepthResources();
    void CleanupTextrueSampler();
    void FreeMemory();
    void CleanupVertexBuffer();
    void CleanupAll();

    inline VkBuffer GetVertexBuffer() const {
        return m_VertexBuffer;
    }

    inline VkBuffer GetIndexBuffer() const {
        return m_IndexBuffer;
    }

    inline const VkDescriptorSet& GetDescriptorSet(uint32_t imageIndex_) const {
        return m_DescriptorSets[imageIndex_];
    }

    uint32_t GetIndicesCount() const;

    inline VkImageView GetDepthImageView() const {
        return m_DepthImageView;
    }

private:

    void CreateImage(uint32_t width_, uint32_t height_, VkFormat format_
        , VkImageTiling tiling_, VkImageUsageFlags usage_, VkMemoryPropertyFlags properties_
        , VkImage& image, VkDeviceMemory& imageMemory);
    bool CreateBuffer(VkDeviceSize bufferSize_, VkBufferUsageFlags usage_
        , VkMemoryPropertyFlags properties, VkBuffer& buffer_, VkDeviceMemory& bufferMemory_);
    uint32_t FindMemoryType(uint32_t typeFilter_, VkMemoryPropertyFlags flags_);
    void CopyBuffer(VkBuffer srcBuffer_, VkBuffer dstBuffer_, VkDeviceSize size_);
    void TransitionImageLayout(VkImage image_, VkFormat format_, VkImageLayout oldLayout_, VkImageLayout newLayout_);
    void CopyBufferToImage(VkBuffer buffer_, VkImage image_, uint32_t width_, uint32_t height_);

    VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates_, VkImageTiling tiling_, VkFormatFeatureFlags features_);
    bool HasStencilComponent(VkFormat format_);

private:

    VulkanContext* m_Context;
    VulkanPipeline* m_Pipeline;
    VulkanFrameBuffer* m_FrameBuffer;
    VulkanSwapchain* m_Swapchain;

    VkBuffer m_VertexBuffer{VK_NULL_HANDLE};
    VkDeviceMemory m_VertexBufferMemory{VK_NULL_HANDLE};
    VkBuffer m_IndexBuffer{VK_NULL_HANDLE};
    VkDeviceMemory m_IndexBufferMemory{VK_NULL_HANDLE};

    VkImage m_TextureImage{VK_NULL_HANDLE};
    VkImageView m_TextureImageView{VK_NULL_HANDLE};
    VkSampler m_Sampler{VK_NULL_HANDLE};
    VkDeviceMemory m_TextureImageMemory{VK_NULL_HANDLE};

    VkDescriptorPool m_DescriptorPool{VK_NULL_HANDLE};
    std::vector<VkDescriptorSet> m_DescriptorSets;

    std::vector<VkBuffer> m_UniformBuffers;
    std::vector<VkDeviceMemory> m_UniformBuffersMemory;
    std::vector<void*> m_UniformBuffersMapped;

    VkImage m_DepthImage{VK_NULL_HANDLE};
    VkDeviceMemory m_DepthImageMemory{VK_NULL_HANDLE};
    VkImageView m_DepthImageView{VK_NULL_HANDLE};
};

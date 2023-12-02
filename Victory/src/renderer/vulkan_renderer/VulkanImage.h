#pragma once

class VulkanContext;
class VulkanFrameBuffer;
class VulkanSwapchain;

struct CreateImageSettings {
    uint32_t Width, Height;
    VkFormat Format;
    VkImageTiling Tiling;
    VkImageUsageFlags Usage;
    VkMemoryPropertyFlags Properties;
    VkSampleCountFlagBits SampleCount;
};

class VulkanImage {
public:

    VulkanImage(VulkanContext* context_, 
        VulkanFrameBuffer* frameBuffer_);

    // Give buffer settings?
    bool LoadTexture(std::string&& path_, CreateImageSettings& settings_);
    bool CreateImage(const CreateImageSettings& settings_);
    bool CreateImageView(VkFormat format_, VkImageAspectFlags aspect_);
    bool CreateSampler();


    void CleanupImage();
    void CleanupImageView();
    void FreeImageMemory();
    void CleanupSampler();
    void CleanupAll();

    inline VkImage GetImage() const {
        return m_Image;
    }

    inline const VkImageView& GetImageView() const {
        return m_ImageView;
    }

    inline VkDeviceMemory GetImageMemory() const {
        return m_ImageMemory;
    }

    inline const VkSampler& GetSampler() const {
        return m_Sampler;
    }

private:

    void CopyBufferToImage(VkBuffer stagingBuffer_);
    void TransitionImageLayout(VkImageLayout oldLayout_, VkImageLayout newLayout_);
    void GenerateMipmaps(VkFormat imageFormat_);

private:

    VulkanContext* m_Context;
    VulkanFrameBuffer* m_FrameBuffer;

    uint32_t m_Width, m_Height;
    uint32_t m_MipLevels{1};

    VkImage m_Image{VK_NULL_HANDLE};
    VkImageView m_ImageView{VK_NULL_HANDLE};
    VkDeviceMemory m_ImageMemory{VK_NULL_HANDLE};

    VkSampler m_Sampler{VK_NULL_HANDLE};
};
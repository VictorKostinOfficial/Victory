#pragma once

#include <string>

namespace Victory 
{
    class VulkanDevice;

    class VulkanImage 
    {
    public:

        VulkanImage(VulkanDevice* vulkanDevice_);

        void CreateImage(const VkImageCreateInfo& imageCI_, 
            const VkMemoryPropertyFlags memoryProperty_);
        void SetImage(VkImage image_);
        void CreateImageView(VkFormat format_, VkImageAspectFlags aspect_);

        void TransitionImageLayout(VkImageLayout oldLayout_, VkImageLayout newLayout_);

        void GenerateMipmaps(VkFormat imageFormat_);

        void CleanupAll();

        inline VkImage GetImage() const {
            return m_Image;
        }

        inline VkImageView GetImageView() const {
            return m_ImageView;
        }

        inline VkDeviceMemory GetImageMemory() const {
            return m_ImageMemory;
        }

    private:

        void CopyBufferToImage(VkBuffer stagingBuffer_);

    private:

        VulkanDevice* m_VulkanDevice;

        VkImage m_Image{ VK_NULL_HANDLE };
        VkImageView m_ImageView{ VK_NULL_HANDLE };
        VkDeviceMemory m_ImageMemory{ VK_NULL_HANDLE };

        VkExtent2D m_ImageExtent;
        uint32_t m_MipLevels{ 1 };
    };
}
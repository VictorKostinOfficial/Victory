#include <vector>
#include <vulkan/vulkan.h>

#include "VulkanImage.h"

#include "VulkanDevice.h"
#include "VulkanUtils.h"

namespace Victory
{

    VulkanImage::VulkanImage(VulkanDevice *vulkanDevice_)
        : m_VulkanDevice{ vulkanDevice_ } {}

    void VulkanImage::CreateImage(const VkImageCreateInfo& imageCI_, 
        const VkMemoryPropertyFlags memoryProperty_) 
    {
        VkDevice device{ m_VulkanDevice->GetDevice() };

        CheckVulkanResult(
            vkCreateImage(device, &imageCI_, nullptr, &m_Image),
            "Image was not created");

        VkMemoryRequirements memReq;
        vkGetImageMemoryRequirements(device, m_Image, &memReq);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = m_VulkanDevice->FindMemoryType(memReq.memoryTypeBits, memoryProperty_);

        CheckVulkanResult(
            vkAllocateMemory(device, &allocInfo, nullptr, &m_ImageMemory),
            "Memor was not allocated");

        CheckVulkanResult(
            vkBindImageMemory(device, m_Image, m_ImageMemory, 0),
            "Image was not binded");
    }

    void VulkanImage::SetImage(VkImage image_)
    {
        m_Image = image_;
    }

    void VulkanImage::CreateImageView(VkFormat format_, VkImageAspectFlags aspect_)
    {
        VkImageViewCreateInfo imageViewCI{};
        imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCI.subresourceRange.baseMipLevel = 0;
        imageViewCI.subresourceRange.levelCount = m_MipLevels;
        imageViewCI.subresourceRange.baseArrayLayer = 0;
        imageViewCI.subresourceRange.layerCount = 1;
        imageViewCI.image = m_Image;
        imageViewCI.format = format_;
        imageViewCI.subresourceRange.aspectMask = aspect_;

        CheckVulkanResult(
            vkCreateImageView(m_VulkanDevice->GetDevice(), 
                &imageViewCI, nullptr, &m_ImageView),
            "Image View was not created");
    }

    void VulkanImage::CleanupAll()
    {
        VkDevice device{ m_VulkanDevice->GetDevice() };
        vkDestroyImageView(device, m_ImageView, nullptr);
        if (m_ImageMemory)
        {
            vkDestroyImage(device, m_Image, nullptr);
            vkFreeMemory(device, m_ImageMemory, nullptr);
        }
    }
}

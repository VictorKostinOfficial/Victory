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
        m_MipLevels = imageCI_.mipLevels;
        m_ImageExtent.width = imageCI_.extent.width;
        m_ImageExtent.height = imageCI_.extent.height;

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

    void VulkanImage::TransitionImageLayout(VkImageLayout oldLayout_, VkImageLayout newLayout_)
    {
        VkCommandBuffer commandBuffer = m_VulkanDevice->BeginSingleTimeCommands();
        {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = oldLayout_;
            barrier.newLayout = newLayout_;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = m_Image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = m_MipLevels;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            VkPipelineStageFlags sourceStage;
            VkPipelineStageFlags destinationStage;

            if (oldLayout_ == VK_IMAGE_LAYOUT_UNDEFINED && newLayout_ == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            } else if (oldLayout_ == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout_ == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            } else if (oldLayout_ == VK_IMAGE_LAYOUT_UNDEFINED && newLayout_ == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            else {
                throw std::invalid_argument("Unsupported layout transition!");
            }

            vkCmdPipelineBarrier( commandBuffer, sourceStage, destinationStage,
                0, 0, nullptr, 0, nullptr, 1, &barrier);
        }
        m_VulkanDevice->EndSingleTimeCommands(commandBuffer);
    }

    void VulkanImage::GenerateMipmaps(VkFormat imageFormat_)
    {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(m_VulkanDevice->GetPhysicalDevice(), 
            imageFormat_, &formatProperties);
    
        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
                throw std::runtime_error("texture image format does not support linear blitting!");
        }

        VkCommandBuffer commandBuffer = m_VulkanDevice->BeginSingleTimeCommands();
        {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.image = m_Image;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.subresourceRange.levelCount = 1;

            int32_t mipWidth = m_ImageExtent.width;
            int32_t mipHeight = m_ImageExtent.height;

            for (uint32_t i{1}; i < m_MipLevels; ++i) {
                barrier.subresourceRange.baseMipLevel = i - 1;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                vkCmdPipelineBarrier(commandBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0, 0, nullptr, 0, nullptr, 1, 
                    &barrier);

                VkImageBlit blit{};
                blit.srcOffsets[0] = {0, 0, 0};
                blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
                blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.srcSubresource.mipLevel = i - 1;
                blit.srcSubresource.baseArrayLayer = 0;
                blit.srcSubresource.layerCount = 1;
                blit.dstOffsets[0] = {0, 0, 0};
                blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth >>= 1 : 1, mipHeight > 1 ? mipHeight >>= 1 : 1, 1 };
                blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.dstSubresource.mipLevel = i;
                blit.dstSubresource.baseArrayLayer = 0;
                blit.dstSubresource.layerCount = 1;

                vkCmdBlitImage(commandBuffer,
                    m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1, &blit, VK_FILTER_LINEAR);

                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                vkCmdPipelineBarrier(commandBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, 
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
                    0, 0, nullptr, 0, nullptr,
                    1, &barrier);
            }

            barrier.subresourceRange.baseMipLevel = m_MipLevels - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);
        }
        m_VulkanDevice->EndSingleTimeCommands(commandBuffer);
    }
}

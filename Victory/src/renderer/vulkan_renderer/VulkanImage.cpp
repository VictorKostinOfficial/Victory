#include <vulkan/vulkan.h>
#include <string>

#include "VulkanImage.h"

#include "VulkanContext.h"
#include "VulkanFrameBuffer.h"
#include "VulkanModel.h"

#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

VulkanImage::VulkanImage(VulkanContext *context_, VulkanFrameBuffer* frameBuffer_) 
    : m_Context{context_}, m_FrameBuffer{frameBuffer_} {
}

VulkanImage::VulkanImage(VulkanContext *context_, VulkanFrameBuffer *frameBuffer_, VkImage image_)
    : m_Context{context_}, m_FrameBuffer{frameBuffer_}, m_Image{image_} {
}

bool VulkanImage::LoadTexture(std::string&& path_, CreateImageSettings& settings_) {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(path_.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    m_Width = static_cast<uint32_t>(texWidth);
    m_Height = static_cast<uint32_t>(texHeight);

    if (!pixels) {
        return false;
    }

    m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    VkDeviceSize imageSize = m_Width * m_Height * 4;
    VkBuffer stagingBuffer{VK_NULL_HANDLE};
    VkDeviceMemory stagingBufferMemory{VK_NULL_HANDLE};
    CreateBufferSettings bufferSettings{};
    bufferSettings.Properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    bufferSettings.Size = imageSize;
    bufferSettings.Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    BindBuffer(m_Context, bufferSettings, stagingBuffer, stagingBufferMemory);
        
    void* data;
    vkMapMemory(m_Context->GetDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(m_Context->GetDevice(), stagingBufferMemory);

    stbi_image_free(pixels);

    settings_.Width = m_Width;
    settings_.Height = m_Height;

    CreateImage(settings_);

    TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        CopyBufferToImage(stagingBuffer);
    // TransitionImageLayout(m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(m_Context->GetDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_Context->GetDevice(), stagingBufferMemory, nullptr);

    GenerateMipmaps(VK_FORMAT_R8G8B8A8_UNORM);
    return true;
}

bool VulkanImage::CreateImage(const CreateImageSettings &settings_, bool bIsNeedTransition) {
    m_Width = settings_.Width;
    m_Height = settings_.Height;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = settings_.Width;
    imageInfo.extent.height = settings_.Height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = m_MipLevels;
    imageInfo.arrayLayers = 1;
    // TODO: use supported by GPU format
    imageInfo.format = settings_.Format;
    imageInfo.tiling = settings_.Tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = settings_.Usage;
    imageInfo.samples = settings_.SampleCount;
    imageInfo.flags = 0;

    vkCreateImage(m_Context->GetDevice(), &imageInfo, nullptr, &m_Image);

    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(m_Context->GetDevice(), m_Image, &memReq);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = m_Context->FindMemoryType(memReq.memoryTypeBits, settings_.Properties);

    vkAllocateMemory(m_Context->GetDevice(), &allocInfo, nullptr, &m_ImageMemory);

    vkBindImageMemory(m_Context->GetDevice(), m_Image, m_ImageMemory, 0);

    if (bIsNeedTransition) {
        TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    return true;
}

bool VulkanImage::CreateImageView(VkFormat format_, VkImageAspectFlags aspect_) {
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

    return (vkCreateImageView(m_Context->GetDevice(), &imageViewCI, nullptr, &m_ImageView) == VK_SUCCESS);
}

bool VulkanImage::CreateSampler() {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(m_Context->GetPhysicalDevice(), &properties);

    VkSamplerCreateInfo samplerCI{};
    samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCI.magFilter = VK_FILTER_LINEAR;
    samplerCI.minFilter = VK_FILTER_LINEAR;
    samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCI.anisotropyEnable = VK_TRUE;
    samplerCI.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerCI.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerCI.unnormalizedCoordinates = VK_FALSE;
    samplerCI.compareEnable = VK_FALSE;
    samplerCI.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCI.minLod = 0.f;
    samplerCI.maxLod = static_cast<float>(m_MipLevels);
    samplerCI.mipLodBias = 0.f;

    return (vkCreateSampler(m_Context->GetDevice(), &samplerCI, nullptr, &m_Sampler) == VK_SUCCESS);
}

void VulkanImage::CleanupImage() {
    vkDestroyImage(m_Context->GetDevice(), m_Image, nullptr);
}

void VulkanImage::CleanupImageView() {
    vkDestroyImageView(m_Context->GetDevice(), m_ImageView, nullptr);
}

void VulkanImage::FreeImageMemory() {
    vkFreeMemory(m_Context->GetDevice(), m_ImageMemory, nullptr);
}

void VulkanImage::CleanupSampler() {
    vkDestroySampler(m_Context->GetDevice(), m_Sampler, nullptr);
}

void VulkanImage::CleanupAll() {
    CleanupSampler();
    CleanupImageView();
    CleanupImage();
    FreeImageMemory();
}

//----------------------------------------------------------------------
//--------------------------------PRIVATE-------------------------------
//----------------------------------------------------------------------
void VulkanImage::CopyBufferToImage(VkBuffer stagingBuffer_) {
    VkCommandBuffer commandBuffer = m_FrameBuffer->BeginSingleTimeCommands();
    {
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            m_Width,
            m_Height,
            1
        };

        vkCmdCopyBufferToImage(
            commandBuffer,
            stagingBuffer_,
            m_Image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );
    }
    m_FrameBuffer->EndSingleTimeCommands(commandBuffer);
}

void VulkanImage::TransitionImageLayout(VkImageLayout oldLayout_, VkImageLayout newLayout_)
{
    VkCommandBuffer commandBuffer = m_FrameBuffer->BeginSingleTimeCommands();
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

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    }
    m_FrameBuffer->EndSingleTimeCommands(commandBuffer);
}

void VulkanImage::GenerateMipmaps(VkFormat imageFormat_) {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(m_Context->GetPhysicalDevice(), 
        imageFormat_, &formatProperties);
    
    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            throw std::runtime_error("texture image format does not support linear blitting!");
    }

    VkCommandBuffer commandBuffer = m_FrameBuffer->BeginSingleTimeCommands();
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

        int32_t mipWidth = m_Width;
        int32_t mipHeight = m_Height;

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
    m_FrameBuffer->EndSingleTimeCommands(commandBuffer);
}

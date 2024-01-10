#include <vector>
#include <vulkan/vulkan.h>

#include "VulkanFrameBuffer.h"
#include "VulkanDevice.h"
#include "VulkanImage.h"
#include "VulkanUtils.h"

namespace Victory 
{
    VulkanFrameBuffer::VulkanFrameBuffer(VulkanDevice* vulkanDevice_) 
        : m_VulkanDevice{ vulkanDevice_ } {}

    void VulkanFrameBuffer::CreateCommandPool(QueueIndex index_) 
    {
        VkCommandPoolCreateInfo commandPoolCI{};
        commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCI.pNext = nullptr;
        commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolCI.queueFamilyIndex = 0;

        CheckVulkanResult(
            vkCreateCommandPool(m_VulkanDevice->GetDevice(), 
                &commandPoolCI, nullptr, &m_CommandPool),
            "Command pool was not created");
    }

    void VulkanFrameBuffer::CreateFrameBufferImages(
        const VkImageCreateInfo &imageCI, const uint32_t frameBufferCount_) 
    {
        m_FrameImages.resize(frameBufferCount_, VulkanImage(m_VulkanDevice));
        for (uint32_t i{ 0 }; i < frameBufferCount_; ++i)
        {
            m_FrameImages[i].CreateImage(imageCI, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            m_FrameImages[i].CreateImageView(imageCI.format, VK_IMAGE_ASPECT_COLOR_BIT);
        }
    }

    void VulkanFrameBuffer::AttachFrameBufferImages(
        std::vector<VkImage> &images_, VkFormat format_)
    {
        const size_t imageCount{ images_.size() };
        m_FrameImages.resize(imageCount, VulkanImage(m_VulkanDevice));
        for (uint32_t i{ 0 }; i < imageCount; ++i)
        {
            m_FrameImages[i].SetImage(images_[i]);
            m_FrameImages[i].CreateImageView(format_, VK_IMAGE_ASPECT_COLOR_BIT);
        }
    }

    void VulkanFrameBuffer::SetAttachments(const std::vector<VulkanImage>& imageViews_)
    {
        m_AttachmentImages = imageViews_;
    }

    void VulkanFrameBuffer::CreateFrameBuffers(VkRenderPass renderPass_, VkExtent2D extent_)
    {
        const size_t frameBuffersCount{ m_FrameImages.size() };
        m_FrameBuffers.resize(frameBuffersCount);
        for (uint32_t i{ 0 }; i < frameBuffersCount; ++i)
        {
            std::vector<VkImageView> attachments;
            attachments.reserve(m_AttachmentImages.size() + 1);
            for (auto&& view : m_AttachmentImages)
            {
                attachments.emplace_back(view.GetImageView());
            }
            attachments.emplace_back(m_FrameImages[i].GetImageView());

            VkFramebufferCreateInfo framebufferCI{};
            framebufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferCI.pNext = nullptr;
            framebufferCI.flags = 0;
            framebufferCI.renderPass = renderPass_;
            framebufferCI.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferCI.pAttachments = attachments.data();
            framebufferCI.width = extent_.width;
            framebufferCI.height = extent_.height;
            framebufferCI.layers = 1;

            CheckVulkanResult(
                vkCreateFramebuffer(m_VulkanDevice->GetDevice(), 
                    &framebufferCI, nullptr, &m_FrameBuffers[i]),
                "Frame buffer was not created");
        }
    }

    void VulkanFrameBuffer::CreateCommandBuffers() {
        VkCommandBufferAllocateInfo allocateI{};
        allocateI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateI.pNext = nullptr;
        allocateI.commandPool = m_CommandPool;
        allocateI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateI.commandBufferCount = static_cast<uint32_t>(m_FrameImages.size());

        m_CommandBuffers.resize(allocateI.commandBufferCount);
        CheckVulkanResult(
            vkAllocateCommandBuffers(m_VulkanDevice->GetDevice(), 
                &allocateI, m_CommandBuffers.data()),
            "Command buffers were not allocated"
        );
    }

    void VulkanFrameBuffer::CleanupAll()
    {
        VkDevice device{ m_VulkanDevice->GetDevice() };
        for (auto&& image : m_AttachmentImages)
        {
            image.CleanupAll();
        }

        for (auto&& image : m_FrameImages)
        {
            image.CleanupAll();
        }

        for (auto&& frameBuffer : m_FrameBuffers)
        {
            vkDestroyFramebuffer(device, frameBuffer, nullptr);
        }

        vkDestroyCommandPool(device, m_CommandPool, nullptr);
    }
}
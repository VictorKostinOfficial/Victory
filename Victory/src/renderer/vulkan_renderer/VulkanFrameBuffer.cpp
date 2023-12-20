#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>

#include "VulkanFrameBuffer.h"

#include <stdexcept>
#include <array>

#include "VulkanContext.h"
#include "VulkanImage.h"

VulkanFrameBuffer::VulkanFrameBuffer(VulkanContext* context_, VkRenderPass renderPass_)
    : m_Context{context_}
    , m_RenderPass{renderPass_} {
}

bool VulkanFrameBuffer::CreateCommandPool(QueueIndex index_) {
    VkCommandPoolCreateInfo commandPoolCI{};
    commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCI.queueFamilyIndex = m_Context->GetQueueIndexes().GetQueueIndex(index_);

    return vkCreateCommandPool(m_Context->GetDevice(), &commandPoolCI, nullptr, &m_CommandPool) == VK_SUCCESS;
}

bool VulkanFrameBuffer::CreateFrameBuffers(const CreateImageSettings& settings_, uint32_t frameBufferCount_) {
    VkFramebufferCreateInfo frameBufferCI{};
    frameBufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCI.renderPass = m_RenderPass;
    frameBufferCI.width = settings_.Width;
    frameBufferCI.height = settings_.Height;
    frameBufferCI.layers = 1;

    VkDevice device = m_Context->GetDevice();

    m_FrameBuffers.resize(frameBufferCount_);
    m_FrameImages.resize(frameBufferCount_, VulkanImage(m_Context, this));

    for (VulkanImage& image : m_FrameImages) {
        image.CreateImage(settings_);
        image.CreateImageView(settings_.Format, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    for(size_t i{0}, n = frameBufferCount_; i < n; ++i) {
        std::vector<VkImageView> attachments;
        attachments.reserve(m_AttachmentImages.size() + 1);
        attachments.emplace_back(m_FrameImages[i].GetImageView());
        for (auto&& image : m_AttachmentImages) {
            attachments.emplace_back(image.GetImageView());
        }

        frameBufferCI.attachmentCount = static_cast<uint32_t>(attachments.size());
        frameBufferCI.pAttachments = attachments.data();

        if (vkCreateFramebuffer(device, &frameBufferCI, nullptr, &m_FrameBuffers[i]) != VK_SUCCESS) {
            printf("\nFrame buffer %zu, was not created!", i);
            return false;
        }
    }

    return true;
}

bool VulkanFrameBuffer::CreateFrameBuffers(const CreateImageSettings &settings_, std::vector<VkImage>& images_) {
    VkFramebufferCreateInfo frameBufferCI{};
    frameBufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCI.renderPass = m_RenderPass;
    frameBufferCI.width = settings_.Width;
    frameBufferCI.height = settings_.Height;
    frameBufferCI.layers = 1;

    VkDevice device = m_Context->GetDevice();

    const size_t imageCount{ images_.size() };
    m_FrameBuffers.resize(imageCount);
    m_FrameImages.reserve(imageCount);

    for (VkImage& image : images_) {
        VulkanImage& newImage{ m_FrameImages.emplace_back(m_Context, this, image) };
        newImage.CreateImageView(settings_.Format, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    for(size_t i{0}, n = imageCount; i < n; ++i) {
        std::vector<VkImageView> attachments;
        attachments.reserve(m_AttachmentImages.size() + 1);
        attachments.emplace_back(m_FrameImages[i].GetImageView());
        for (auto&& image : m_AttachmentImages) {
            attachments.emplace_back(image.GetImageView());
        }

        frameBufferCI.attachmentCount = static_cast<uint32_t>(attachments.size());
        frameBufferCI.pAttachments = attachments.data();

        if (vkCreateFramebuffer(device, &frameBufferCI, nullptr, &m_FrameBuffers[i]) != VK_SUCCESS) {
            printf("\nFrame buffer %zu, was not created!", i);
            return false;
        }
    }

    return true;
}

bool VulkanFrameBuffer::CreateCommandBuffer(uint32_t commandBufferCount_) {
    VkCommandBufferAllocateInfo commandBufferAllocInfo{};
    commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocInfo.commandPool = m_CommandPool;
    commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocInfo.commandBufferCount = commandBufferCount_;

    m_CommandBuffers.resize(commandBufferCount_);
    return vkAllocateCommandBuffers(m_Context->GetDevice(), &commandBufferAllocInfo, m_CommandBuffers.data()) == VK_SUCCESS;
}

void VulkanFrameBuffer::AddAttachment(const CreateImageSettings settings_) {
    VulkanImage& newAttachment = m_AttachmentImages.emplace_back(m_Context, this);

    newAttachment.CreateImage(settings_);
    newAttachment.CreateImageView(settings_.Format, VK_IMAGE_ASPECT_DEPTH_BIT);
}

// bool VulkanFrameBuffer::CreateDepthResources(VkFormat depthFormat_, const VkExtent2D& extent_) {
//     m_DepthImage = new VulkanImage(m_Context, this);
//     CreateImageSettings settings{};
//     settings.Width = extent_.width;
//     settings.Height = extent_.height;
//     settings.Format = depthFormat_;
//     settings.Tiling = VK_IMAGE_TILING_OPTIMAL;
//     settings.Usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
//     settings.Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
//     // TODO: Dynamic change msaa
//     settings.SampleCount = m_Context->GetSampleCount();

//     m_DepthImage->CreateImage(settings);
//     m_DepthImage->CreateImageView(settings.Format, VK_IMAGE_ASPECT_DEPTH_BIT);
//     return true;
// }

// bool VulkanFrameBuffer::CreateColorResources(VkFormat colorFormat_, const VkExtent2D& extent_) {
//     m_ColorImage = new VulkanImage(m_Context, this);
//     CreateImageSettings settings{};
//     settings.Width = extent_.width;
//     settings.Height = extent_.height;
//     settings.Format = colorFormat_;
//     settings.Tiling = VK_IMAGE_TILING_OPTIMAL;
//     settings.Usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
//     settings.Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
//     // TODO: Dynamic change msaa
//     settings.SampleCount = m_Context->GetSampleCount();

//     m_ColorImage->CreateImage(settings);
//     m_ColorImage->CreateImageView(settings.Format, VK_IMAGE_ASPECT_COLOR_BIT);
//     return true;
// }

VkCommandBuffer VulkanFrameBuffer::BeginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_CommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_Context->GetDevice(), &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void VulkanFrameBuffer::EndSingleTimeCommands(VkCommandBuffer commandBuffer_) {
    vkEndCommandBuffer(commandBuffer_);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer_;

    // TODO: submit via transfer queue, need Transfer Command pool
    // https://vulkan-tutorial.com/Vertex_buffers/Staging_buffer

    VkQueue queue;
    m_Context->GetQueue(queue, QueueIndex::eGraphics);
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(m_Context->GetDevice(), m_CommandPool, 1, &commandBuffer_);
}

void VulkanFrameBuffer::CleanupFrameBuffers(bool cleanupAll_) {
    for (auto&& frameBuffer : m_FrameBuffers) {
        vkDestroyFramebuffer(m_Context->GetDevice(), frameBuffer, nullptr);
    }
    m_FrameBuffers.clear();

    for (auto&& image : m_FrameImages) {
        image.CleanupImageView();
        if (cleanupAll_) {
            image.CleanupImage();
            image.FreeImageMemory();
        }
    }
    m_FrameImages.clear();
}

void VulkanFrameBuffer::CleanupCommandPool() {
    vkDestroyCommandPool(m_Context->GetDevice(), m_CommandPool, nullptr);
}

void VulkanFrameBuffer::CleanupAll(bool cleanupAll_) {
    CleanupFrameBuffers(cleanupAll_);
    CleanupCommandPool();
}

//----------------------------------------------------------------------
//--------------------------------PRIVATE-------------------------------
//----------------------------------------------------------------------
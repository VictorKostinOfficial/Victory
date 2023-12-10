#include <vulkan/vulkan.h>
#include <vector>

#include "VulkanFrameBuffer.h"

#include <stdexcept>
#include <array>

#include "VulkanContext.h"
#include "VulkanImage.h"

VulkanFrameBuffer::VulkanFrameBuffer(VulkanContext* context_)
    : m_Context{context_} {
}

bool VulkanFrameBuffer::CreateFrameBuffers(VkRenderPass pass_, const VkExtent2D& extent_, 
        const std::vector<VulkanImage>& images_, const bool isImGui_ /* = false */) {
    VkFramebufferCreateInfo frameBufferCI{};
    frameBufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCI.renderPass = pass_;
    frameBufferCI.width = extent_.width;
    frameBufferCI.height = extent_.height;
    frameBufferCI.layers = 1;

    auto&& device = m_Context->GetDevice();
    m_FrameBuffers.resize(images_.size());
    // auto&& images = m_Swapchain->GetImages();
    // m_FrameBuffers.resize(images.size());

    for(size_t i{0}, n = images_.size(); i < n; ++i) {

        std::vector<VkImageView> attachments;
        if (isImGui_) {
            attachments.emplace_back(images_[i].GetImageView());
        } else {
            attachments.emplace_back(m_ColorImage->GetImageView());
            attachments.emplace_back(m_DepthImage->GetImageView());
            attachments.emplace_back(images_[i].GetImageView());
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
    commandBufferAllocInfo.commandPool = m_Context->GetCommandPool();
    commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocInfo.commandBufferCount = commandBufferCount_;

    m_CommandBuffers.resize(commandBufferCount_);
    return vkAllocateCommandBuffers(m_Context->GetDevice(), &commandBufferAllocInfo, m_CommandBuffers.data()) == VK_SUCCESS;
}

// TODO: Create one fucntion for Resuouce creation
bool VulkanFrameBuffer::CreateDepthResources(VkFormat depthFormat_, const VkExtent2D& extent_) {
    m_DepthImage = new VulkanImage(m_Context, this);
    CreateImageSettings settings{};
    settings.Width = extent_.width;
    settings.Height = extent_.height;
    settings.Format = depthFormat_;
    settings.Tiling = VK_IMAGE_TILING_OPTIMAL;
    settings.Usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    settings.Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    // TODO: Dynamic change msaa
    settings.SampleCount = m_Context->GetSampleCount();

    m_DepthImage->CreateImage(settings);
    m_DepthImage->CreateImageView(settings.Format, VK_IMAGE_ASPECT_DEPTH_BIT);
    return true;
}

bool VulkanFrameBuffer::CreateColorResources(VkFormat colorFormat_, const VkExtent2D& extent_) {
    m_ColorImage = new VulkanImage(m_Context, this);
    CreateImageSettings settings{};
    settings.Width = extent_.width;
    settings.Height = extent_.height;
    settings.Format = colorFormat_;
    settings.Tiling = VK_IMAGE_TILING_OPTIMAL;
    settings.Usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    settings.Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    // TODO: Dynamic change msaa
    settings.SampleCount = m_Context->GetSampleCount();

    m_ColorImage->CreateImage(settings);
    m_ColorImage->CreateImageView(settings.Format, VK_IMAGE_ASPECT_COLOR_BIT);
    return true;
}

VkCommandBuffer VulkanFrameBuffer::BeginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_Context->GetCommandPool();
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

    vkFreeCommandBuffers(m_Context->GetDevice(), m_Context->GetCommandPool(), 1, &commandBuffer_);
}

void VulkanFrameBuffer::CleanupFrameBuffers() {
    for (auto&& frameBuffer : m_FrameBuffers) {
        vkDestroyFramebuffer(m_Context->GetDevice(), frameBuffer, nullptr);
    }
}

void VulkanFrameBuffer::CleanupDepthResources() {
    m_DepthImage->CleanupAll();
}

void VulkanFrameBuffer::CleanupColorResources() {
    m_ColorImage->CleanupAll();
}

void VulkanFrameBuffer::CleanupAll() {
    CleanupColorResources();
    CleanupDepthResources();
    CleanupFrameBuffers();
}

//----------------------------------------------------------------------
//--------------------------------PRIVATE-------------------------------
//----------------------------------------------------------------------
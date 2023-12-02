#include <vulkan/vulkan.h>
#include <vector>

#include "VulkanFrameBuffer.h"

#include <stdexcept>
#include <array>

#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanPipeline.h"
#include "VulkanImage.h"

VulkanFrameBuffer::VulkanFrameBuffer(VulkanContext* context_, VulkanSwapchain* swapchain_, 
    VulkanPipeline* pipeline_) 
    : m_Context{context_}
    , m_Swapchain{swapchain_}
    , m_Pipeline{pipeline_} {
}

bool VulkanFrameBuffer::CreateFrameBuffers() {
    VkFramebufferCreateInfo frameBufferCI{};
    frameBufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCI.renderPass = m_Pipeline->GetRenderPass();
    frameBufferCI.width = m_Swapchain->GetExtent().width;
    frameBufferCI.height = m_Swapchain->GetExtent().height;
    frameBufferCI.layers = 1;

    auto&& device = m_Context->GetDevice();
    auto&& imageViews = m_Swapchain->GetImageViews();
    m_FrameBuffers.resize(imageViews.size());

    for(size_t i{0}, n = imageViews.size(); i < n; ++i) {
        std::array<VkImageView, 2> attachmetns{imageViews[i], m_DepthImage->GetImageView()};
        frameBufferCI.attachmentCount = static_cast<uint32_t>(attachmetns.size());
        frameBufferCI.pAttachments = attachmetns.data();

        if (vkCreateFramebuffer(device, &frameBufferCI, nullptr, &m_FrameBuffers[i]) != VK_SUCCESS) {
            printf("\nFrame buffer %zu, was not created!", i);
            return false;
        }
    }

    return true;
}

bool VulkanFrameBuffer::CreateCommandPool() {
    VkCommandPoolCreateInfo commandPoolCI{};
    commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCI.queueFamilyIndex = m_Context->GetQueueIndexes().GetQueueIndex(QueueIndex::eGraphics);

    return vkCreateCommandPool(m_Context->GetDevice(), &commandPoolCI, nullptr, &m_CommandPool) == VK_SUCCESS;
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

bool VulkanFrameBuffer::CreateDepthResources() {
    m_DepthImage = new VulkanImage(m_Context, this);
    CreateImageSettings settings{};
    settings.Width = m_Swapchain->GetExtent().width;
    settings.Height = m_Swapchain->GetExtent().height;
    settings.Format = m_Pipeline->FindDepthFormat();
    settings.Tiling = VK_IMAGE_TILING_OPTIMAL;
    settings.Usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    settings.Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    // TODO: i don't need png here
    m_DepthImage->CreateImage(settings);
    m_DepthImage->CreateImageView(settings.Format, VK_IMAGE_ASPECT_DEPTH_BIT);
    return true;
}


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

    VkQueue queue = m_Context->GetQueue(QueueIndex::eGraphics);
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(m_Context->GetDevice(), m_CommandPool, 1, &commandBuffer_);
}

void VulkanFrameBuffer::CleanupFrameBuffers() {
    for (auto&& frameBuffer : m_FrameBuffers) {
        vkDestroyFramebuffer(m_Context->GetDevice(), frameBuffer, nullptr);
    }
}

void VulkanFrameBuffer::CleanupDepthResources() {
    m_DepthImage->CleanupAll();
}

void VulkanFrameBuffer::CleanupCommandPool()
{
    vkDestroyCommandPool(m_Context->GetDevice(), m_CommandPool, nullptr);
}

void VulkanFrameBuffer::CleanupAll() {
    CleanupDepthResources();
    CleanupCommandPool();
    CleanupFrameBuffers();
}

//----------------------------------------------------------------------
//--------------------------------PRIVATE-------------------------------
//----------------------------------------------------------------------
#include "VulkanFrameBuffer.h"

bool VulkanFrameBuffer::CreateFrameBuffers(VulkanContext &context_, VulkanSwapchain &swapchain_, VulkanPipeline &pipeline_) {
    VkFramebufferCreateInfo frameBufferCI{};
    frameBufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCI.renderPass = pipeline_.GetRenderPass();
    frameBufferCI.width = swapchain_.GetExtent().width;
    frameBufferCI.height = swapchain_.GetExtent().height;
    frameBufferCI.layers = 1;

    VkDevice device = context_.GetDevice();

    auto&& imageViews = swapchain_.GetImageViews();
    m_FrameBuffers.resize(imageViews.size());
    for(size_t i{0}, n = imageViews.size(); i < n; ++i) {
        frameBufferCI.attachmentCount = 1;
        frameBufferCI.pAttachments = &imageViews[i];

        if (vkCreateFramebuffer(device, &frameBufferCI, nullptr, &m_FrameBuffers[i]) != VK_SUCCESS) {
            printf("\nFrame buffer %lu, was not created!", i);
            return false;
        }
    }

    return true;
}

bool VulkanFrameBuffer::CreateCommandPool(VulkanContext &context_) {
    VkCommandPoolCreateInfo commandPoolCI{};
    commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCI.queueFamilyIndex = context_.GetQueueIndexes().GetQueueIndex(QueueIndex::eGraphics);

    VkDevice device = context_.GetDevice();
    return vkCreateCommandPool(device, &commandPoolCI, nullptr, &m_CommandPool) == VK_SUCCESS;
}

bool VulkanFrameBuffer::CreateCommandBuffer(VkDevice device_, uint32_t commandBufferCount_) {
    VkCommandBufferAllocateInfo commandBufferAllocInfo{};
    commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocInfo.commandPool = m_CommandPool;
    commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocInfo.commandBufferCount = commandBufferCount_;

    m_CommandBuffers.resize(commandBufferCount_);
    return vkAllocateCommandBuffers(device_, &commandBufferAllocInfo, m_CommandBuffers.data()) == VK_SUCCESS;
}

void VulkanFrameBuffer::RecordCommandBuffer(VulkanSwapchain& swapchain_, VulkanPipeline& pipeline_, uint32_t commandBufferIndex_, uint32_t imageIndex_) {
    VkCommandBufferBeginInfo commandBufferBI{};
    commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkCommandBuffer commandBuffer = m_CommandBuffers[commandBufferIndex_];
    vkBeginCommandBuffer(commandBuffer, &commandBufferBI);

    VkRect2D rect{};
    rect.offset = {0, 0};
    rect.extent = swapchain_.GetExtent();

    VkClearColorValue clearColorValue{48.f / 255.f, 10.f / 255.f, 36.f / 255.f, 1.f};
    VkClearValue clearValue{clearColorValue};

    VkRenderPassBeginInfo renderPassBI{};
    renderPassBI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBI.renderPass = pipeline_.GetRenderPass();
    renderPassBI.framebuffer = m_FrameBuffers[imageIndex_];
    renderPassBI.renderArea = rect;
    renderPassBI.clearValueCount = 1;
    renderPassBI.pClearValues = &clearValue;

    // TODO: split function begin/end
    vkCmdBeginRenderPass(commandBuffer, &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.GetPipeline());

        VkViewport viewport{};
        viewport.x = 0.f;
        viewport.y = 0.f;
        viewport.width = static_cast<float>(rect.extent.width);
        viewport.height = static_cast<float>(rect.extent.height);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &rect);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    }
    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);
}

void VulkanFrameBuffer::CleanupFrameBuffers(VkDevice device_) {
    for (auto&& frameBuffer : m_FrameBuffers) {
        vkDestroyFramebuffer(device_, frameBuffer, nullptr);
    }
}

void VulkanFrameBuffer::CleanupCommandPool(VkDevice device_) {
    vkDestroyCommandPool(device_, m_CommandPool, nullptr);
}

void VulkanFrameBuffer::CleanupAll(VulkanContext &context_) {
    VkDevice device = context_.GetDevice();
    CleanupCommandPool(device);
    CleanupFrameBuffers(device);
}

VkCommandBuffer VulkanFrameBuffer::GetCommandBuffer(uint32_t commandBufferIndex) const {
    return m_CommandBuffers[commandBufferIndex];
}

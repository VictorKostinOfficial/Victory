#include "VulkanFrameBuffer.h"

bool VulkanFrameBuffer::CreateFrameBuffers(VulkanContext &context_, 
    VulkanSwapchain &swapchain_, VulkanPipeline &pipeline_,
    VulkanBuffer& buffer_) {
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
        std::array<VkImageView, 2> attachmetns{imageViews[i], buffer_.GetDepthImageView()};
        frameBufferCI.attachmentCount = static_cast<uint32_t>(attachmetns.size());
        frameBufferCI.pAttachments = attachmetns.data();

        if (vkCreateFramebuffer(device, &frameBufferCI, nullptr, &m_FrameBuffers[i]) != VK_SUCCESS) {
            printf("\nFrame buffer %zu, was not created!", i);
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

void VulkanFrameBuffer::RecordCommandBuffer(VulkanSwapchain& swapchain_, VulkanPipeline& pipeline_, uint32_t commandBufferIndex_, uint32_t imageIndex_, VulkanBuffer& vertexBuffer_) {
    VkCommandBufferBeginInfo commandBufferBI{};
    commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkCommandBuffer commandBuffer = m_CommandBuffers[commandBufferIndex_];
    vkBeginCommandBuffer(commandBuffer, &commandBufferBI);

    VkRect2D rect{};
    rect.offset = {0, 0};
    rect.extent = swapchain_.GetExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{48.f / 255.f, 10.f / 255.f, 36.f / 255.f, 1.f}};
    clearValues[1].depthStencil = {1.f, 0};

    VkRenderPassBeginInfo renderPassBI{};
    renderPassBI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBI.renderPass = pipeline_.GetRenderPass();
    renderPassBI.framebuffer = m_FrameBuffers[imageIndex_];
    renderPassBI.renderArea = rect;
    renderPassBI.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBI.pClearValues = clearValues.data();

    // TODO: split function begin/end
    vkCmdBeginRenderPass(commandBuffer, &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.GetPipeline());

        std::vector<VkBuffer> vertexBuffers{vertexBuffer_.GetVertexBuffer()};
        VkBuffer indexBuffer{vertexBuffer_.GetIndexBuffer()};
        std::vector<VkDeviceSize> offsets{0};

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets.data());
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        VkViewport viewport{};
        viewport.x = 0.f;
        viewport.y = 0.f;
        viewport.width = static_cast<float>(rect.extent.width);
        viewport.height = static_cast<float>(rect.extent.height);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &rect);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.GetPipelineLayout(), 0, 1, &vertexBuffer_.GetDescriptorSet(commandBufferIndex_), 0, nullptr);

        vkCmdDrawIndexed(commandBuffer, vertexBuffer_.GetIndicesCount(), 1, 0, 0, 0);

    }
    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);
}

VkCommandBuffer VulkanFrameBuffer::BeginSingleTimeCommands(VkDevice device_) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_CommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void VulkanFrameBuffer::EndSingleTimeCommands(VkCommandBuffer commandBuffer_, VulkanContext* context_) {
    vkEndCommandBuffer(commandBuffer_);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer_;

    // TODO: submit via transfer queue, need Transfer Command pool
    // https://vulkan-tutorial.com/Vertex_buffers/Staging_buffer

    VkQueue queue = context_->GetQueue(QueueIndex::eGraphics);
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(context_->GetDevice(), m_CommandPool, 1, &commandBuffer_);
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

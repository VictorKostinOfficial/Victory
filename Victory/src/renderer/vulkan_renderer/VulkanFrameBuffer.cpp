#include "VulkanFrameBuffer.h"

#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanPipeline.h"
#include "VulkanBuffer.h"

VulkanFrameBuffer::VulkanFrameBuffer(VulkanContext* context_, VulkanSwapchain* swapchain_, 
    VulkanPipeline* pipeline_, VulkanBuffer* buffer_) 
    : m_Context{context_}
    , m_Swapchain{swapchain_}
    , m_Pipeline{pipeline_}
    , m_Buffer{buffer_} {
}

void VulkanFrameBuffer::SetVulkanBuffer(VulkanBuffer *buffer_) {
    m_Buffer = buffer_;
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
        std::array<VkImageView, 2> attachmetns{imageViews[i], m_Buffer->GetDepthImageView()};
        frameBufferCI.attachmentCount = static_cast<uint32_t>(attachmetns.size());
        frameBufferCI.pAttachments = attachmetns.data();

        if (vkCreateFramebuffer(device, &frameBufferCI, nullptr, &m_FrameBuffers[i]) != VK_SUCCESS) {
            printf("\nFrame buffer %zu, was not created!", i);
            return false;
        }
    }

    return true;
}

bool VulkanFrameBuffer::CreateCommandPool()
{
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

void VulkanFrameBuffer::RecordCommandBuffer(uint32_t commandBufferIndex_, uint32_t imageIndex_) {
    VkCommandBufferBeginInfo commandBufferBI{};
    commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkCommandBuffer commandBuffer = m_CommandBuffers[commandBufferIndex_];
    vkBeginCommandBuffer(commandBuffer, &commandBufferBI);

    VkRect2D rect{};
    rect.offset = {0, 0};
    rect.extent = m_Swapchain->GetExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{48.f / 255.f, 10.f / 255.f, 36.f / 255.f, 1.f}};
    clearValues[1].depthStencil = {1.f, 0};

    VkRenderPassBeginInfo renderPassBI{};
    renderPassBI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBI.renderPass = m_Pipeline->GetRenderPass();
    renderPassBI.framebuffer = m_FrameBuffers[imageIndex_];
    renderPassBI.renderArea = rect;
    renderPassBI.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBI.pClearValues = clearValues.data();

    // TODO: split function begin/end
    vkCmdBeginRenderPass(commandBuffer, &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->GetPipeline());

        std::vector<VkBuffer> vertexBuffers{m_Buffer->GetVertexBuffer()};
        VkBuffer indexBuffer{m_Buffer->GetIndexBuffer()};
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

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->GetPipelineLayout(), 
            0, 1, &m_Buffer->GetDescriptorSet(commandBufferIndex_), 0, nullptr);

        vkCmdDrawIndexed(commandBuffer, m_Buffer->GetIndicesCount(), 1, 0, 0, 0);

    }
    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);
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

void VulkanFrameBuffer::CleanupCommandPool() {
    vkDestroyCommandPool(m_Context->GetDevice(), m_CommandPool, nullptr);
}

void VulkanFrameBuffer::CleanupAll() {
    CleanupCommandPool();
    CleanupFrameBuffers();
}

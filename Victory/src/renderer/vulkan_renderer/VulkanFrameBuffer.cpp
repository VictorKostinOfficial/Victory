#include "VulkanFrameBuffer.h"
#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanPipeline.h"

bool VulkanFrameBuffer::CreateFrameBuffer(VulkanContext* context_, VulkanSwapchain* swapchain_, VulkanPipeline* pipeline_) {
    vk::FramebufferCreateInfo frameBufferCI{};
    frameBufferCI.setRenderPass(pipeline_->GetRenderPass());
    frameBufferCI.setAttachments(swapchain_->GetImageViews());
    frameBufferCI.setWidth(swapchain_->GetExtent().width);
    frameBufferCI.setWidth(swapchain_->GetExtent().height);
    frameBufferCI.setLayers(1);

    vk::Device device = context_->GetDevice();
    m_FrameBuffer = device.createFramebuffer(frameBufferCI);
    return true;
}

bool VulkanFrameBuffer::CreateCommandPool(VulkanContext* context_) {
    vk::CommandPoolCreateInfo commandPoolCI{};
    commandPoolCI.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    commandPoolCI.setQueueFamilyIndex(context_->GetQueueIndex(0));

    vk::Device device = context_->GetDevice();
    m_CommandPool = device.createCommandPool(commandPoolCI);
    return true;
}

bool VulkanFrameBuffer::CreateCommandBuffer(VulkanContext* contex_) {
    vk::CommandBufferAllocateInfo commandBufferAllocInfo{};
    commandBufferAllocInfo.setCommandPool(m_CommandPool);
    commandBufferAllocInfo.setLevel(vk::CommandBufferLevel::ePrimary);
    commandBufferAllocInfo.setCommandBufferCount(1);

    vk::Device device = contex_->GetDevice();
    m_CommandBuffers = device.allocateCommandBuffers(commandBufferAllocInfo);
    return true;
}

void VulkanFrameBuffer::RecordCommandBuffer(VulkanSwapchain* swapchain_, VulkanPipeline* pipeline_, uint32_t imageIndex_) {
    vk::CommandBufferBeginInfo commandBufferBI{};
    m_CommandBuffers[imageIndex_].begin(commandBufferBI);

    vk::Rect2D rect{};
    rect.setOffset({0,0});
    rect.setExtent(swapchain_->GetExtent());

    vk::ClearColorValue clearColorValue{0.45f, 0.55f, 0.6f, 1.f};
    vk::ClearValue clearValue{clearColorValue};

    vk::RenderPassBeginInfo renderPassBI{};
    renderPassBI.setRenderPass(pipeline_->GetRenderPass());
    renderPassBI.setFramebuffer(m_FrameBuffer);
    renderPassBI.setRenderArea(rect);
    renderPassBI.setClearValues(clearValue);

    // TODO: split function begin/end
    m_CommandBuffers[imageIndex_].beginRenderPass(renderPassBI, vk::SubpassContents::eInline);

    m_CommandBuffers[imageIndex_].bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_->GetPipeline());

    vk::Viewport viewport{};
    viewport.setX(0.f);
    viewport.setY(0.f);
    viewport.setWidth(static_cast<float>(rect.extent.width));
    viewport.setHeight(static_cast<float>(rect.extent.height));
    viewport.setMinDepth(0.f);
    viewport.setMaxDepth(1.f);

    m_CommandBuffers[imageIndex_].setViewport(0, viewport);
    m_CommandBuffers[imageIndex_].setScissor(0, rect);

    m_CommandBuffers[imageIndex_].draw(3, 1, 0, 0);

    m_CommandBuffers[imageIndex_].endRenderPass();

    m_CommandBuffers[imageIndex_].end();
}

void VulkanFrameBuffer::Cleanup(VulkanContext* context_) {
    vk::Device device = context_->GetDevice();

    device.destroyCommandPool(m_CommandPool);
    device.destroyFramebuffer(m_FrameBuffer);
}

#include "VulkanFrameBuffer.h"
#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanPipeline.h"

bool VulkanFrameBuffer::CreateFrameBuffer(VulkanContext* context_, VulkanSwapchain* swapchain_, VulkanPipeline* pipeline_) {
    vk::FramebufferCreateInfo frameBufferCI{};
    frameBufferCI.setRenderPass(pipeline_->GetRenderPass());
    frameBufferCI.setWidth(swapchain_->GetExtent().width);
    frameBufferCI.setHeight(swapchain_->GetExtent().height);
    frameBufferCI.setLayers(1);

    vk::Device device = context_->GetDevice();
    auto&& imageViews = swapchain_->GetImageViews();
    for(auto&& imageView : imageViews) {
        frameBufferCI.setAttachments(imageView);
        m_FrameBuffers.emplace_back(device.createFramebuffer(frameBufferCI));
    }

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

bool VulkanFrameBuffer::CreateCommandBuffer(VulkanContext* contex_, uint32_t commandBufferCount_) {
    vk::CommandBufferAllocateInfo commandBufferAllocInfo{};
    commandBufferAllocInfo.setCommandPool(m_CommandPool);
    commandBufferAllocInfo.setLevel(vk::CommandBufferLevel::ePrimary);
    commandBufferAllocInfo.setCommandBufferCount(commandBufferCount_);

    vk::Device device = contex_->GetDevice();
    m_CommandBuffers = device.allocateCommandBuffers(commandBufferAllocInfo);
    return true;
}

void VulkanFrameBuffer::RecordCommandBuffer(VulkanSwapchain* swapchain_, VulkanPipeline* pipeline_, uint32_t commandBufferIndex_, uint32_t imageIndex_) {
    vk::CommandBufferBeginInfo commandBufferBI{};
    vk::CommandBuffer commandBuffer = m_CommandBuffers[commandBufferIndex_];
    commandBuffer.begin(commandBufferBI);

    vk::Rect2D rect{};
    rect.setOffset({0,0});
    rect.setExtent(swapchain_->GetExtent());

    vk::ClearColorValue clearColorValue{48.f / 255.f, 10.f / 255.f, 36.f / 255.f, 1.f};
    
    vk::ClearValue clearValue{clearColorValue};

    vk::RenderPassBeginInfo renderPassBI{};
    renderPassBI.setRenderPass(pipeline_->GetRenderPass());
    renderPassBI.setFramebuffer(m_FrameBuffers[imageIndex_]);
    renderPassBI.setRenderArea(rect);
    renderPassBI.setClearValues(clearValue);

    // TODO: split function begin/end
    commandBuffer.beginRenderPass(renderPassBI, vk::SubpassContents::eInline);
    {
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_->GetPipeline());

        vk::Viewport viewport{};
        viewport.setX(0.f);
        viewport.setY(0.f);
        viewport.setWidth(static_cast<float>(rect.extent.width));
        viewport.setHeight(static_cast<float>(rect.extent.height));
        viewport.setMinDepth(0.f);
        viewport.setMaxDepth(1.f);

        commandBuffer.setViewport(0, viewport);
        commandBuffer.setScissor(0, rect);

        commandBuffer.draw(3, 1, 0, 0);

        commandBuffer.endRenderPass();

    }
    commandBuffer.end();
}

void VulkanFrameBuffer::CleanupFrameBuffers(VulkanContext* context_) {
    vk::Device device = context_->GetDevice();

    for (auto&& frameBuffer : m_FrameBuffers) {
        device.destroyFramebuffer(frameBuffer);
    }
}

void VulkanFrameBuffer::Cleanup(VulkanContext* context_) {
    vk::Device device = context_->GetDevice();

    // TODO: need refactor
    device.destroyCommandPool(m_CommandPool);
    CleanupFrameBuffers(context_);
}

const vk::CommandBuffer VulkanFrameBuffer::GetCommandBuffer(uint32_t index_) const {
    return m_CommandBuffers[index_];
}

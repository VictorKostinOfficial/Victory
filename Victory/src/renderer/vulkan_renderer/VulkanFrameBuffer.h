#pragma once

#include <vulkan/vulkan.hpp>

class VulkanContext;
class VulkanSwapchain;
class VulkanPipeline;

class VulkanFrameBuffer
{
public:

    bool CreateFrameBuffer(VulkanContext* context_, VulkanSwapchain* swapchain_, VulkanPipeline* pipeline_);
    bool CreateCommandPool(VulkanContext* context_);
    bool CreateCommandBuffer(VulkanContext* contex_);
    void RecordCommandBuffer(VulkanSwapchain* swapchain_, VulkanPipeline* pipeline_, uint32_t imageIndex_);
    void Cleanup(VulkanContext* context_);

    const vk::CommandBuffer GetCommandBuffer() const;

private:

    std::vector<vk::Framebuffer> m_FrameBuffers;
    vk::CommandPool m_CommandPool{VK_NULL_HANDLE};
    vk::CommandBuffer m_CommandBuffer{VK_NULL_HANDLE};
};

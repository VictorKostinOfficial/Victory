#pragma once

#include <vulkan/vulkan.hpp>

class VulkanContext;
class VulkanSwapchain;
class VulkanPipeline;

class VulkanFrameBuffers
{
public:

    bool CreateFrameBuffers(VulkanContext* context_);

private:

    std::vector<vk::Framebuffer> m_FrameBuffers;
};

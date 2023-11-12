#pragma once

#include <vulkan/vulkan.hpp>

class VulkanSwapchain {
    vk::SwapchainKHR m_Swapchain{VK_NULL_HANDLE};

    std::vector<vk::Image> m_Images;
    std::vector<vk::ImageView> m_ImageViews;
    std::vector<vk::Framebuffer> m_FrameBuffer;

    uint32_t m_Width;
    uint32_t m_Height;
};
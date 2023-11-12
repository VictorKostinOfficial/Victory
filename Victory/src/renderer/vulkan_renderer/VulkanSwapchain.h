#pragma once

#include <vulkan/vulkan.hpp>

class GLFWwindow;
class VulkanContext;

class VulkanSwapchain {
public:

    bool CreateSurface(VulkanContext* context_, GLFWwindow* window_);
    bool CreateSwapchain(VulkanContext* context_, GLFWwindow* window_);
    void Cleanup(VulkanContext* context_);

    vk::SurfaceKHR GetSurface();
    vk::Format GetSwapcahinFormat();

private:

    vk::SurfaceFormatKHR ChooseSwapchainSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats_);
    vk::PresentModeKHR ChoosePresentationModeFormat(const std::vector<vk::PresentModeKHR>& modes_);
    vk::Extent2D ChooseSwapchainExtent(const vk::SurfaceCapabilitiesKHR& capabilities_, GLFWwindow* window_);

private:

    vk::SurfaceKHR m_Surface{VK_NULL_HANDLE};
    vk::SwapchainKHR m_Swapchain{VK_NULL_HANDLE};

    std::vector<vk::Image> m_Images;
    std::vector<vk::ImageView> m_ImageViews;
    std::vector<vk::Framebuffer> m_FrameBuffer;

    uint32_t m_Width;
    uint32_t m_Height;
};
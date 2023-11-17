#pragma once

#include <vulkan/vulkan.hpp>

struct GLFWwindow;
class VulkanContext;
class VulkanFrameBuffer;
class VulkanPipeline;

class VulkanSwapchain {
public:

    // Mb store context inside the class
    bool CreateSurface(VulkanContext* context_, GLFWwindow* window_);
    bool CreateSwapchain(VulkanContext* context_, GLFWwindow* window_);
    bool RecreateSwapchain(VulkanContext* context_, GLFWwindow* window_, VulkanFrameBuffer* buffer_, VulkanPipeline* pipeline_);
    bool CreateImageViews(VulkanContext* context_);
    void CleanupSwapchain(VulkanContext* context_, VulkanFrameBuffer* buffer_);
    void Cleanup(VulkanContext* context_);

    const vk::SurfaceKHR GetSurface() const;
    const vk::SwapchainKHR GetSwapchain() const;
    vk::Format GetSwapchainFormat() const;
    const vk::Extent2D GetExtent() const ;
    const std::vector<vk::ImageView>& GetImageViews() const;

private:

    vk::SurfaceFormatKHR ChooseSwapchainSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats_);
    vk::PresentModeKHR ChoosePresentationModeFormat(const std::vector<vk::PresentModeKHR>& modes_);
    vk::Extent2D ChooseSwapchainExtent(const vk::SurfaceCapabilitiesKHR& capabilities_, GLFWwindow* window_);

private:

    vk::SurfaceKHR m_Surface{VK_NULL_HANDLE};
    vk::SwapchainKHR m_Swapchain{VK_NULL_HANDLE};

    vk::SurfaceFormatKHR m_SurfaceFormat;
    vk::PresentModeKHR m_PresentMode;
    vk::Extent2D m_Extent;

    std::vector<vk::Image> m_Images;
    std::vector<vk::ImageView> m_ImageViews;
    std::vector<vk::Framebuffer> m_FrameBuffer;

    uint32_t m_Width;
    uint32_t m_Height;
};
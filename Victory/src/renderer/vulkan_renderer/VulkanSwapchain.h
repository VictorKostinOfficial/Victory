#pragma once

#include <vulkan/vulkan_core.h>

#include "VulkanContext.h"

struct GLFWwindow;

class VulkanSwapchain {
public:

    bool CreateSurface(VkInstance instance_, GLFWwindow* window_);
    bool CreateSwapchain(VulkanContext& context_, GLFWwindow* window_);
    bool CreateImageViews(VulkanContext& context_);

    void CleanupImageViews(VkDevice device_);
    void CleanupSwapchain(VkDevice device_);
    void CleanupSurface(VkInstance instance_);
    void CleanupAll(VulkanContext& context_);

    VkSurfaceKHR GetSurface() const;
    VkSwapchainKHR GetSwapchain() const;
    VkSurfaceFormatKHR GetSurfaceFormat() const;
    VkExtent2D GetExtent() const;
    const std::vector<VkImageView>& GetImageViews() const;

private:

    void ChooseSwapchainExtent(VkSurfaceCapabilitiesKHR capabilities_, GLFWwindow* window_);
    bool ChooseSwapchainSurfaceFormat(VkPhysicalDevice phDevice_);
    bool ChoosePresentationModeFormat(VkPhysicalDevice phDevice_);

private:

    VkSurfaceKHR m_Surface{VK_NULL_HANDLE};
    VkSwapchainKHR m_Swapchain{VK_NULL_HANDLE};

    VkExtent2D m_Extent;
    VkSurfaceFormatKHR m_SurfaceFormat;
    VkPresentModeKHR m_PresentMode;

    std::vector<VkImage> m_Images;
    std::vector<VkImageView> m_ImageViews;
    std::vector<VkFramebuffer> m_FrameBuffer;
};

#pragma once

class VulkanContext;
class VulkanFrameBuffer;
class VulkanImage;

struct GLFWwindow;

class VulkanSwapchain {
public:

    VulkanSwapchain(VulkanContext* context_);

    bool CreateSurface(GLFWwindow* window_);
    bool CreateSwapchain(GLFWwindow* window_);
    void GetImages(std::vector<VkImage>& images_);

    void CleanupSwapchain();
    void CleanupSurface();
    void CleanupAll();

    VkSurfaceKHR GetSurface() const;
    VkSwapchainKHR GetSwapchain() const;
    VkSurfaceFormatKHR GetSurfaceFormat() const;
    VkExtent2D GetExtent() const;

    inline const uint32_t GetImageCount() const {
        return m_ImageCount;
    }

private:

    void ChooseSwapchainExtent(VkSurfaceCapabilitiesKHR capabilities_, GLFWwindow* window_);
    bool ChooseSwapchainSurfaceFormat();
    bool ChoosePresentationModeFormat();

private:

    VulkanContext* m_Context;

    VkSurfaceKHR m_Surface{VK_NULL_HANDLE};
    VkSwapchainKHR m_Swapchain{VK_NULL_HANDLE};

    VkExtent2D m_Extent;
    VkSurfaceFormatKHR m_SurfaceFormat;
    VkPresentModeKHR m_PresentMode;

    uint32_t m_ImageCount;
};

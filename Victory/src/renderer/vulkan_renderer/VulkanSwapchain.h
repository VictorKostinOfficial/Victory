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
    bool CreateImages();
    bool CreateImageViews(const VkImageAspectFlags aspectFlags_);
    bool CreateSamplers();
    // bool CreateImageViews();
    // bool CreateImageView(VkImage imgage_, VkFormat format_, 
    //     VkImageAspectFlags aspectFlags_, VkImageView& imageView_);

    // void CleanupImageViews();
    void CleanupSamplers();
    void CleanupImageViews();
    void CleanupImages();
    void CleanupSwapchain();
    void CleanupSurface();
    void CleanupAll();

    VkSurfaceKHR GetSurface() const;
    VkSwapchainKHR GetSwapchain() const;
    VkSurfaceFormatKHR GetSurfaceFormat() const;
    VkExtent2D GetExtent() const;
    // const std::vector<VkImageView>& GetImageViews() const;

    inline const std::vector<VulkanImage>& GetImages() const {
        return m_Images;
    }

    inline const uint32_t GetImageCount() const {
        return m_ImageCount;
    }

private:

    void ChooseSwapchainExtent(VkSurfaceCapabilitiesKHR capabilities_, GLFWwindow* window_);
    bool ChooseSwapchainSurfaceFormat();
    bool ChoosePresentationModeFormat();

private:

    VulkanContext* m_Context;
    VulkanFrameBuffer* m_FrameBuffer;

    VkSurfaceKHR m_Surface{VK_NULL_HANDLE};
    VkSwapchainKHR m_Swapchain{VK_NULL_HANDLE};

    VkExtent2D m_Extent;
    VkSurfaceFormatKHR m_SurfaceFormat;
    VkPresentModeKHR m_PresentMode;

    // TODO: make list of VulkanImages
    uint32_t m_ImageCount;
    std::vector<VulkanImage> m_Images;
    // std::vector<VkImage> m_Images;
    // std::vector<VkImageView> m_ImageViews;
};

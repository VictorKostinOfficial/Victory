#pragma once

struct GLFWwindow;

namespace Victory {

    class VulkanDevice;

    class VulkanSwapchain {
    public:

        static VulkanSwapchain* Init(VulkanDevice* vulkanDevice_ = nullptr, 
            GLFWwindow* window_ = nullptr);
        static void Cleanup();

        void CreateSurface();
        void CreateSwapchain();

        void RecreateSwapchain();

        inline VkSurfaceKHR GetSurface() const {
            return m_Surface;
        }

        inline VkSwapchainKHR GetSwapchain() const {
            return m_Swapchain;
        }

        inline VkSurfaceFormatKHR GetSurfaceFormat() const {
            return m_SurfaceFormat;
        }

        inline VkExtent2D GetExtent() const {
            return m_Extent;
        }

        inline const uint32_t GetImageCount() const {
            return m_ImageCount;
        }

    private:

        VulkanSwapchain(VulkanDevice* vulkanDevice_,
            GLFWwindow* window_);
        ~VulkanSwapchain() = default;

        void CleanupResourses();

        void ChooseSwapchainExtent(VkSurfaceCapabilitiesKHR capabilities_);
        void ChooseSwapchainSurfaceFormat();
        void ChoosePresentationModeFormat();

    private:

        GLFWwindow* m_Window{ nullptr };

        VulkanDevice* m_VulkanDevice{ nullptr };

        VkSurfaceKHR m_Surface{ VK_NULL_HANDLE };
        VkSwapchainKHR m_Swapchain{ VK_NULL_HANDLE };

        VkExtent2D m_Extent;
        VkSurfaceFormatKHR m_SurfaceFormat;
        VkPresentModeKHR m_PresentMode;

        uint32_t m_ImageCount;
    };
}
#include <vulkan/vulkan.h>

#include "VulkanSwapchain.h"

#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <algorithm>

#include "VulkanDevice.h"
#include "VulkanUtils.h"

namespace Victory{

    static VulkanSwapchain* s_VulkanSwapcahinInstance{ nullptr };

    VulkanSwapchain* VulkanSwapchain::Init(VulkanDevice *vulkanDevice_, 
            GLFWwindow* window_) {
        std::cout << "Init VulkanSwapchain" << std::endl;
        if (s_VulkanSwapcahinInstance) {
            return s_VulkanSwapcahinInstance;
        }

        s_VulkanSwapcahinInstance = new VulkanSwapchain(vulkanDevice_, window_);

        return s_VulkanSwapcahinInstance;
    }

    void VulkanSwapchain::Cleanup() {
        std::cout << "Cleanup VulkanSwapchain" << std::endl;
        s_VulkanSwapcahinInstance->CleanupResourses();

        delete s_VulkanSwapcahinInstance;
    }

    VulkanSwapchain::VulkanSwapchain(VulkanDevice* vulkanDevice_, 
        GLFWwindow* window_) 
        : m_VulkanDevice{ vulkanDevice_}
        , m_Window{window_} {}

    void VulkanSwapchain::CleanupResourses() {
        auto&& device{ m_VulkanDevice->GetDevice() };
        auto&& instance{ m_VulkanDevice->GetInstance() };

        vkDestroySwapchainKHR(device, m_Swapchain, nullptr);
        vkDestroySurfaceKHR(instance, m_Surface, nullptr);
    }

    void VulkanSwapchain::CreateSurface() {
        if (glfwCreateWindowSurface(m_VulkanDevice->GetInstance(), m_Window, nullptr, &m_Surface) != VK_SUCCESS) {
            glfwDestroyWindow(m_Window);
            glfwTerminate();
            throw std::runtime_error("Surface was not created");
        }
    }

    void VulkanSwapchain::CreateSwapchain() {
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_VulkanDevice->GetPhysicalDevice(), m_Surface, &capabilities);

        ChooseSwapchainExtent(capabilities);
        ChooseSwapchainSurfaceFormat();
        ChoosePresentationModeFormat();

        m_ImageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && m_ImageCount > capabilities.maxImageCount) {
            m_ImageCount = capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR swapchainCI{};
        swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCI.minImageCount = m_ImageCount;
        swapchainCI.surface = m_Surface;
        swapchainCI.imageFormat = m_SurfaceFormat.format;
        swapchainCI.imageColorSpace = m_SurfaceFormat.colorSpace;
        swapchainCI.imageExtent = m_Extent;
        swapchainCI.imageArrayLayers = 1;
        swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
        // TODO: Comiute and Transfer queue family?
        const VulkanQueueIndices indices{ m_VulkanDevice->GetQueueIndices() };
        const std::vector<uint32_t> queueFamilyIndexes{
            indices.graphicsQueueIndex,
            indices.presentQueueIndex
        };

        if (queueFamilyIndexes[0] != queueFamilyIndexes[1]) {
            swapchainCI.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapchainCI.queueFamilyIndexCount = 2;
            swapchainCI.pQueueFamilyIndices = queueFamilyIndexes.data();
        } else {
            swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swapchainCI.queueFamilyIndexCount = 1;
            swapchainCI.pQueueFamilyIndices = &queueFamilyIndexes[0];
        }

        swapchainCI.preTransform = capabilities.currentTransform;
        swapchainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCI.presentMode = m_PresentMode;
        swapchainCI.clipped = VK_TRUE;
        swapchainCI.oldSwapchain = VK_NULL_HANDLE;

        CheckVulkanResult(vkCreateSwapchainKHR(m_VulkanDevice->GetDevice(), &swapchainCI, nullptr, &m_Swapchain),
            "Swapchain was not created");
    }

    void VulkanSwapchain::RecreateSwapchain() {
        VkDevice device = m_VulkanDevice->GetDevice();
        vkDeviceWaitIdle(device);
        vkDestroySwapchainKHR(device, m_Swapchain, nullptr);
        CreateSwapchain();
    }

    void VulkanSwapchain::ChooseSwapchainExtent(VkSurfaceCapabilitiesKHR capabilities_) {
        if (capabilities_.currentExtent.width != UINT32_MAX) {
            m_Extent = capabilities_.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(m_Window, &height, &width);

            VkExtent2D actualExtent{
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, 
                capabilities_.minImageExtent.width, capabilities_.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, 
                capabilities_.minImageExtent.height, capabilities_.maxImageExtent.height);

            m_Extent = actualExtent;
        }
    }

    void VulkanSwapchain::ChooseSwapchainSurfaceFormat() {
        uint32_t surfaceFormatsCount{0};
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_VulkanDevice->GetPhysicalDevice(), 
            m_Surface, &surfaceFormatsCount, nullptr);

        if (surfaceFormatsCount == 0) {
            throw std::runtime_error("Surface formats count is 0!");
        }

        std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatsCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_VulkanDevice->GetPhysicalDevice(),
            m_Surface, &surfaceFormatsCount, surfaceFormats.data());

        if (surfaceFormats.empty()) {
            throw std::runtime_error("Surface formats is empty");
        }

        for (auto&& format : surfaceFormats) {
            if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
                format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                m_SurfaceFormat = format;
                return;
            }
        }

        m_SurfaceFormat = surfaceFormats[0];
    }

    void VulkanSwapchain::ChoosePresentationModeFormat() {
        uint32_t presentModesCount{0};
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_VulkanDevice->GetPhysicalDevice(), 
            m_Surface, &presentModesCount, nullptr);

        if (presentModesCount == 0) {
            throw std::runtime_error("Present modes count is 0!");
        }

        std::vector<VkPresentModeKHR> presentModes(presentModesCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_VulkanDevice->GetPhysicalDevice(), 
            m_Surface, &presentModesCount, presentModes.data());

        if (presentModes.empty()) {
            throw std::runtime_error("\nPresent modes is empty");
        }

        for (auto&& mode : presentModes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                m_PresentMode = mode;
                return;
            }
        }

        m_PresentMode = VK_PRESENT_MODE_FIFO_KHR;
    }
}
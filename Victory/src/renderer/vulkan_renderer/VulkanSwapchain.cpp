#include <vulkan/vulkan.h>
#include <vector>

#include "VulkanSwapchain.h"
#include "VulkanFrameBuffer.h"
#include "VulkanImage.h"

#include <GLFW/glfw3.h>
#include <stdio.h>
#include <algorithm>

#include "VulkanContext.h"

VulkanSwapchain::VulkanSwapchain(VulkanContext* context_)
    : m_Context{context_} {
}

bool VulkanSwapchain::CreateSurface(GLFWwindow *window_)
{
    if (glfwCreateWindowSurface(m_Context->GetInstance(), window_, nullptr, &m_Surface) != VK_SUCCESS) {
        glfwDestroyWindow(window_);
        glfwTerminate();
        return false;
    }

    return true;
}

bool VulkanSwapchain::CreateSwapchain(GLFWwindow* window_) {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Context->GetPhysicalDevice(), m_Surface, &capabilities);

    ChooseSwapchainExtent(capabilities, window_);
    if (!ChooseSwapchainSurfaceFormat()) {
        return false;
    }
    if (!ChoosePresentationModeFormat()) {
        return false;
    }

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
    swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // If i want postprocess need value like VK_IMAGE_USAGE_TRANSFER_DST_BIT
    
    // TODO: Comiute and Transfer queue family?
    QueueIndexes queueIndexes = m_Context->GetQueueIndexes();
    std::vector<uint32_t> queueFamilyIndexes{
        queueIndexes.GetQueueIndex(QueueIndex::eGraphics), 
        queueIndexes.GetQueueIndex(QueueIndex::ePresent)
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

    auto&& device = m_Context->GetDevice();
    if (vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &m_Swapchain) != VK_SUCCESS) {
        return false;
    }
    return true;
}

void VulkanSwapchain::GetImages(std::vector<VkImage> &images_) {
    if (vkGetSwapchainImagesKHR(m_Context->GetDevice(), m_Swapchain, &m_ImageCount, images_.data()) != VK_SUCCESS) {
        printf("\nSwapchain images were not geted!");
    }
}

void VulkanSwapchain::CleanupSwapchain() {
    vkDestroySwapchainKHR(m_Context->GetDevice(), m_Swapchain, nullptr);
}

void VulkanSwapchain::CleanupSurface() {
    vkDestroySurfaceKHR(m_Context->GetInstance(), m_Surface, nullptr);
}

void VulkanSwapchain::CleanupAll() {
    CleanupSwapchain();
    CleanupSurface();
}

VkSurfaceKHR VulkanSwapchain::GetSurface() const {
    return m_Surface;
}

VkSwapchainKHR VulkanSwapchain::GetSwapchain() const {
    return m_Swapchain;
}

VkSurfaceFormatKHR VulkanSwapchain::GetSurfaceFormat() const {
    return m_SurfaceFormat;
}

VkExtent2D VulkanSwapchain::GetExtent() const {
    return m_Extent;
}

//----------------------------------------------------------------------
//--------------------------------PRIVATE-------------------------------
//----------------------------------------------------------------------
void VulkanSwapchain::ChooseSwapchainExtent(VkSurfaceCapabilitiesKHR capabilities_, GLFWwindow *window_) {
    if (capabilities_.currentExtent.width != UINT32_MAX) {
        m_Extent = capabilities_.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window_, &height, &width);

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

bool VulkanSwapchain::ChooseSwapchainSurfaceFormat()
{
    uint32_t surfaceFormatsCount{0};
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_Context->GetPhysicalDevice(), 
        m_Surface, &surfaceFormatsCount, nullptr);

    if (surfaceFormatsCount == 0) {
        printf("\nSurface formats count is 0!");
        return false;
    }

    std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatsCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_Context->GetPhysicalDevice(),
         m_Surface, &surfaceFormatsCount, surfaceFormats.data());

    if (surfaceFormats.empty()) {
        printf("\nSurface formats is empty");
        return false;
    }

    for (auto&& format : surfaceFormats) {
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            m_SurfaceFormat = format;
            return true;
        }
    }

    m_SurfaceFormat = surfaceFormats[0];
    return true;
}

bool VulkanSwapchain::ChoosePresentationModeFormat() {
    uint32_t presentModesCount{0};
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_Context->GetPhysicalDevice(), 
        m_Surface, &presentModesCount, nullptr);

    if (presentModesCount == 0) {
        printf("\nPresent modes count is 0!");
        return false;
    }

    std::vector<VkPresentModeKHR> presentModes(presentModesCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_Context->GetPhysicalDevice(), 
        m_Surface, &presentModesCount, presentModes.data());

    if (presentModes.empty()) {
        printf("\nPresent modes is empty");
        return false;
    }

    for (auto&& mode : presentModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            m_PresentMode = mode;
            return true;
        }
    }

    m_PresentMode = VK_PRESENT_MODE_FIFO_KHR;
    return true;
}

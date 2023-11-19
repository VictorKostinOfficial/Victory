#include "VulkanSwapchain.h"

#include <GLFW/glfw3.h>

bool VulkanSwapchain::CreateSurface(VkInstance instance_, GLFWwindow* window_) {
    if (glfwCreateWindowSurface(instance_, window_, nullptr, &m_Surface) != VK_SUCCESS) {
        glfwDestroyWindow(window_);
        glfwTerminate();
        return false;
    }

    return true;
}

bool VulkanSwapchain::CreateSwapchain(VulkanContext& context_, GLFWwindow* window_) {
    VkPhysicalDevice phDevice = context_.GetPhysicalDevice();

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phDevice, m_Surface, &capabilities);

    ChooseSwapchainExtent(capabilities, window_);
    if (!ChooseSwapchainSurfaceFormat(phDevice)) {
        return false;
    }
    if (!ChoosePresentationModeFormat(phDevice)) {
        return false;
    }

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    m_Images.reserve(imageCount);
    m_ImageViews.reserve(imageCount);

    VkSwapchainCreateInfoKHR swapchainCI{};
    swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCI.minImageCount = imageCount;
    swapchainCI.surface = m_Surface;
    swapchainCI.imageFormat = m_SurfaceFormat.format;
    swapchainCI.imageColorSpace = m_SurfaceFormat.colorSpace;
    swapchainCI.imageExtent = m_Extent;
    swapchainCI.imageArrayLayers = 1;
    swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // If i want postprocess need value like VK_IMAGE_USAGE_TRANSFER_DST_BIT
    
    // TODO: Comiute and Transfer queue family?
    QueueIndexes queueIndexes = context_.GetQueueIndexes();
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

    VkDevice device = context_.GetDevice();
    if (vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &m_Swapchain) != VK_SUCCESS) {
        return false;
    }

    m_Images.resize(imageCount);
    if (vkGetSwapchainImagesKHR(device, m_Swapchain, &imageCount, m_Images.data()) != VK_SUCCESS) {
        printf("\nSwapchain images were not geted!");
        return false;
    }

    return true;
}

bool VulkanSwapchain::CreateImageViews(VulkanContext& context_) {
    VkDevice device = context_.GetDevice();

    m_ImageViews.resize(m_Images.size());
    VkImageViewCreateInfo imageViewCI{};
    imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewCI.subresourceRange.baseArrayLayer = 0;
    imageViewCI.subresourceRange.levelCount = 1;
    imageViewCI.subresourceRange.baseArrayLayer = 0;
    imageViewCI.subresourceRange.layerCount = 1;
    for (size_t i{0}, n = m_Images.size(); i < n; ++i) {
        imageViewCI.image = m_Images[i];
        imageViewCI.format = m_SurfaceFormat.format;

        if (vkCreateImageView(device, &imageViewCI, nullptr, &m_ImageViews[i]) != VK_SUCCESS) {
            printf("\nImage view with index %lu was not created!", i);
            return false;
        }
    }
    return true;
}

void VulkanSwapchain::CleanupImageViews(VkDevice device_) {
    for (auto&& imageView : m_ImageViews) {
        vkDestroyImageView(device_, imageView, nullptr);
    }
}

void VulkanSwapchain::CleanupSwapchain(VkDevice device_) {
    vkDestroySwapchainKHR(device_, m_Swapchain, nullptr);
}

void VulkanSwapchain::CleanupSurface(VkInstance instance_) {
    vkDestroySurfaceKHR(instance_, m_Surface, nullptr);
}

void VulkanSwapchain::CleanupAll(VulkanContext& context_) {
    CleanupImageViews(context_.GetDevice());
    CleanupSwapchain(context_.GetDevice());
    CleanupSurface(context_.GetInstance());
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

const std::vector<VkImageView>& VulkanSwapchain::GetImageViews() const{
    return m_ImageViews;
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

bool VulkanSwapchain::ChooseSwapchainSurfaceFormat(VkPhysicalDevice phDevice_) {
    uint32_t surfaceFormatsCount{0};
    vkGetPhysicalDeviceSurfaceFormatsKHR(phDevice_, m_Surface, &surfaceFormatsCount, nullptr);

    if (surfaceFormatsCount == 0) {
        printf("\nSurface formats count is 0!");
        return false;
    }

    std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatsCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(phDevice_, m_Surface, &surfaceFormatsCount, surfaceFormats.data());

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

bool VulkanSwapchain::ChoosePresentationModeFormat(VkPhysicalDevice phDevice_) {
    uint32_t presentModesCount{0};
    vkGetPhysicalDeviceSurfacePresentModesKHR(phDevice_, m_Surface, &presentModesCount, nullptr);

    if (presentModesCount == 0) {
        printf("\nPresent modes count is 0!");
        return false;
    }

    std::vector<VkPresentModeKHR> presentModes(presentModesCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(phDevice_, m_Surface, &presentModesCount, presentModes.data());

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


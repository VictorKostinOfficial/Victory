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

bool VulkanSwapchain::CreateImages(VulkanFrameBuffer* frameBuffer_) {

    std::vector<VkImage> images(m_ImageCount);
    images.resize(m_ImageCount);
    if (vkGetSwapchainImagesKHR(m_Context->GetDevice(), m_Swapchain, &m_ImageCount, images.data()) != VK_SUCCESS) {
        printf("\nSwapchain images were not geted!");
        return false;
    }

    m_Images.clear();
    for (auto&& image : images) {
        m_Images.push_back(VulkanImage(m_Context, frameBuffer_, image));
    }
    return true;

    // for (auto&& image : m_Images) {
    //     CreateImageSettings settings{};
    //     settings.Width = m_Extent.width;
    //     settings.Height = m_Extent.height;
    //     settings.Format = m_SurfaceFormat.format;
    //     settings.Tiling = VK_IMAGE_TILING_LINEAR;
    //     settings.Usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    //     settings.Properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    //     settings.SampleCount = VK_SAMPLE_COUNT_1_BIT;
    //     image.CreateImage(settings);
    // }
}

bool VulkanSwapchain::CreateImageViews(const VkImageAspectFlags aspectFlags_) {
    for (auto&& image : m_Images) {
        image.CreateImageView(m_SurfaceFormat.format, aspectFlags_);
    }
    return true;
}

bool VulkanSwapchain::CreateSamplers() {
    for (auto&& image : m_Images) {
        image.CreateSampler();
    }
    return true;
}

// bool VulkanSwapchain::CreateImageViews() {
//     m_ImageViews.resize(m_Images.size());
//     for (size_t i{0}, n = m_Images.size(); i < n ; ++i) {
//         if (!CreateImageView(m_Images[i], m_SurfaceFormat.format, 
//             VK_IMAGE_ASPECT_COLOR_BIT, m_ImageViews[i])) {
//             return false;
//         }
//     }
//     return true;
// }

// bool VulkanSwapchain::CreateImageView(VkImage imgage_, VkFormat format_, 
//     VkImageAspectFlags aspectFlags_, VkImageView& imageView_) {
//     VkImageViewCreateInfo imageViewCI{};
//     imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
//     imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
//     imageViewCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
//     imageViewCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
//     imageViewCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
//     imageViewCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
//     imageViewCI.subresourceRange.baseArrayLayer = 0;
//     imageViewCI.subresourceRange.levelCount = 1;
//     imageViewCI.subresourceRange.baseArrayLayer = 0;
//     imageViewCI.subresourceRange.layerCount = 1;

//     imageViewCI.image = imgage_;
//     imageViewCI.format = format_;
//     imageViewCI.subresourceRange.aspectMask = aspectFlags_;

//     return (vkCreateImageView(m_Context->GetDevice(), &imageViewCI, nullptr, &imageView_) == VK_SUCCESS);
// }

// void VulkanSwapchain::CleanupImageViews() {
//     auto&& device = m_Context->GetDevice();
//     for (auto&& imageView : m_ImageViews) {
//         vkDestroyImageView(device, imageView, nullptr);
//     }
// }

void VulkanSwapchain::CleanupSamplers() {
    auto&& device = m_Context->GetDevice();
    for (auto&& image : m_Images) {
        image.CleanupSampler();
    }
}

void VulkanSwapchain::CleanupImageViews()
{
    auto&& device = m_Context->GetDevice();
    for (auto&& image : m_Images) {
        image.CleanupImageView();
    }
}

void VulkanSwapchain::CleanupImages() {
    auto&& device = m_Context->GetDevice();
    for (auto&& image : m_Images) {
        image.CleanupImage();
    }
}

void VulkanSwapchain::CleanupSwapchain() {
    vkDestroySwapchainKHR(m_Context->GetDevice(), m_Swapchain, nullptr);
}

void VulkanSwapchain::CleanupSurface() {
    vkDestroySurfaceKHR(m_Context->GetInstance(), m_Surface, nullptr);
}

void VulkanSwapchain::CleanupAll() {
    CleanupSamplers();
    CleanupImageViews();
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

// const std::vector<VkImageView>& VulkanSwapchain::GetImageViews() const{
//     return m_ImageViews;
// }

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

bool VulkanSwapchain::ChooseSwapchainSurfaceFormat() {
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


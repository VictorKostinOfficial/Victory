#include "VulkanSwapchain.h"
#include "VulkanContext.h"

#include <GLFW/glfw3.h>

#include <algorithm>

bool VulkanSwapchain::CreateSurface(VulkanContext *context_, GLFWwindow* window_) {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(static_cast<VkInstance>(context_->GetInstance()), window_, nullptr, &surface)) {
        glfwDestroyWindow(window_);
        glfwTerminate();
        printf("\nGLFW surface was not created");
        return false;
    }

    m_Surface = static_cast<vk::SurfaceKHR>(surface);
    return true;
}

bool VulkanSwapchain::CreateSwapchain(VulkanContext *context_, GLFWwindow *window_) {
    vk::PhysicalDevice phDevice = context_->GetPhysicalDevice();

    vk::SurfaceCapabilitiesKHR capabilities = phDevice.getSurfaceCapabilitiesKHR(m_Surface);
    std::vector<vk::SurfaceFormatKHR> surfaceFormats = phDevice.getSurfaceFormatsKHR(m_Surface);
    std::vector<vk::PresentModeKHR> presentModes = phDevice.getSurfacePresentModesKHR(m_Surface);

    if (presentModes.empty() || surfaceFormats.empty()) {
        printf("\nPresent modes or Surface formats are empty");
        return false;
    }

    m_SurfaceFormat = ChooseSwapchainSurfaceFormat(surfaceFormats);
    m_PresentMode = ChoosePresentationModeFormat(presentModes);
    m_Extent = ChooseSwapchainExtent(capabilities, window_);

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    m_Images.reserve(imageCount);
    m_ImageViews.reserve(imageCount);

    vk::SwapchainCreateInfoKHR swapchainCI{{},
        m_Surface,
        imageCount,
        m_SurfaceFormat.format,
        m_SurfaceFormat.colorSpace,
        m_Extent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment
        // If i want postprocess need value like VK_IMAGE_USAGE_TRANSFER_DST_BIT
    };

    // TODO: Compute and Transfer queue family?
    std::vector<uint32_t> queueFamilyIndices = {context_->GetQueueIndex(0), context_->GetQueueIndex(1)};
    if (queueFamilyIndices[0] != queueFamilyIndices [1]) {
        swapchainCI.imageSharingMode = vk::SharingMode::eConcurrent;
        swapchainCI.setQueueFamilyIndices(queueFamilyIndices);
    } else {
        swapchainCI.imageSharingMode = vk::SharingMode::eExclusive;
        swapchainCI.queueFamilyIndexCount = 1;
        swapchainCI.pQueueFamilyIndices = &queueFamilyIndices[0];
    }

    swapchainCI.preTransform = capabilities.currentTransform;
    swapchainCI.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    swapchainCI.presentMode = m_PresentMode;
    swapchainCI.clipped = VK_TRUE;
    swapchainCI.oldSwapchain = VK_NULL_HANDLE;

    m_Swapchain = context_->GetDevice().createSwapchainKHR(swapchainCI);
    m_Images = context_->GetDevice().getSwapchainImagesKHR(m_Swapchain);
    return true;
}

bool VulkanSwapchain::CreateImageViews(VulkanContext *context_) {
    vk::Device device = context_->GetDevice();
    vk::ImageViewCreateInfo imageViewCI{};
    for (auto&& image : m_Images) {
        imageViewCI.image = image;
        imageViewCI.viewType = vk::ImageViewType::e2D;
        imageViewCI.format = m_SurfaceFormat.format;
        imageViewCI.components.r = vk::ComponentSwizzle::eIdentity;
        imageViewCI.components.g = vk::ComponentSwizzle::eIdentity;
        imageViewCI.components.b = vk::ComponentSwizzle::eIdentity;
        imageViewCI.components.a = vk::ComponentSwizzle::eIdentity;
        imageViewCI.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imageViewCI.subresourceRange.baseArrayLayer = 0;
        imageViewCI.subresourceRange.levelCount = 0;
        imageViewCI.subresourceRange.baseArrayLayer = 0;
        imageViewCI.subresourceRange.layerCount = 0;

        vk::ImageView imageView = device.createImageView(imageViewCI);
        m_ImageViews.emplace_back(imageView);
    }
    return false;
}

void VulkanSwapchain::Cleanup(VulkanContext *context_) {
    vk::Instance instance = context_->GetInstance();
    vk::Device device = context_->GetDevice();
    for (auto&& imageView : m_ImageViews) {
        device.destroyImageView(imageView);
    }
    device.destroySwapchainKHR(m_Swapchain);
    instance.destroySurfaceKHR(m_Surface);
}

vk::SurfaceKHR VulkanSwapchain::GetSurface() {
    return m_Surface;
}

vk::Format VulkanSwapchain::GetSwapchainFormat() {
    return m_SurfaceFormat.format;
}

vk::Extent2D VulkanSwapchain::GetExtent() {
    return m_Extent;
}

vk::SurfaceFormatKHR VulkanSwapchain::ChooseSwapchainSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats_) {
    for (auto&& format : formats_) {
        if (format.format == vk::Format::eB8G8R8A8Srgb &&
            format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return format;
        }
    }

    return formats_[0];
}

vk::PresentModeKHR VulkanSwapchain::ChoosePresentationModeFormat(const std::vector<vk::PresentModeKHR>& modes_) {
    for (auto&& mode : modes_) {
        if (mode == vk::PresentModeKHR::eMailbox) {
            return mode;
        }
    }

    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D VulkanSwapchain::ChooseSwapchainExtent(const vk::SurfaceCapabilitiesKHR &capabilities_, GLFWwindow *window_) {
    if (capabilities_.currentExtent.width != UINT32_MAX) {
        return capabilities_.currentExtent;
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

        return actualExtent;
    }
}
#include <vector>
#include <vulkan/vulkan.h>

#include "VulkanDevice.h"

#include <iostream>
#include <unordered_set>
#include <GLFW/glfw3.h>

#include "VulkanUtils.h"

namespace Victory 
{

    static VulkanDevice* s_VulkanDeviceInstance{ nullptr };

    VulkanDevice* VulkanDevice::Init() 
    {
        std::cout << "Init VulkanDevice" << std::endl;
        if (s_VulkanDeviceInstance) 
        {
            return s_VulkanDeviceInstance;
        }

        s_VulkanDeviceInstance = new VulkanDevice();

        return s_VulkanDeviceInstance;
    }

    void VulkanDevice::Cleanup() 
    {
        std::cout << "Cleanup VulkanDevice" << std::endl;

        s_VulkanDeviceInstance->CleanupResourses();

        delete s_VulkanDeviceInstance;
    }

    const uint32_t VulkanDevice::FindMemoryType(const uint32_t typeFilter_, 
        const VkMemoryPropertyFlags flags_) const 
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) 
        {
            if ((typeFilter_ & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & flags_) == flags_) 
            {
                return i;
            }
        }
        return UINT32_MAX;
    }

    void VulkanDevice::CleanupResourses()
    {
        vkDestroyDevice(m_Device, nullptr);
        vkDestroyInstance(m_Instance, nullptr);
    }

    const VkFormat VulkanDevice::FindDepthFormat()
    {
        return FindSupportedFormat(
			{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    const VkFormat VulkanDevice::FindSupportedFormat(const std::vector<VkFormat> &formats, 
        VkImageTiling tiling_, VkFormatFeatureFlags features_)
    {
        for (auto&& format : formats)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);

			if (tiling_ == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features_) == features_)
			{
				return format;
			}
			else if (tiling_ == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features_) == features_)
			{
				return format;
			}
		}

		throw std::runtime_error("failed to find supported format!");
    }

    void CollectLayers(std::vector<const char*> &layers_) 
    {
#ifndef NDEBUG
        layers_.emplace_back("VK_LAYER_KHRONOS_validation");
#endif // NDEBUG
    }

    void CollectExtensions(std::vector<const char*>& extensions_) 
    {
#ifndef NDEBUG
        extensions_.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif // NDEBUG

        uint32_t glfwExtensionCount{ 0 };
        const char** glfwExtensions{ glfwGetRequiredInstanceExtensions(&glfwExtensionCount) };
        extensions_.reserve(glfwExtensionCount + 1);
        for (uint32_t i{0}; i < glfwExtensionCount; ++i) 
        {
            extensions_.emplace_back(glfwExtensions[i]);
        }
    }

    void VulkanDevice::CreateInstance() 
    {
        VkApplicationInfo applicationI{};
        applicationI.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        applicationI.pNext = nullptr;
        applicationI.pApplicationName = "Victory Playground";
        applicationI.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        applicationI.pEngineName = "Victory";
        applicationI.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        applicationI.apiVersion = VK_API_VERSION_1_3;

        std::vector<const char*> layers;
        CollectLayers(layers);

        std::vector<const char*> extensions;
        CollectExtensions(extensions);

        VkInstanceCreateInfo instanceCI{};
        instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCI.pNext = nullptr;
        instanceCI.flags = 0;
        instanceCI.pApplicationInfo = &applicationI;
        instanceCI.enabledLayerCount = static_cast<uint32_t>(layers.size());
        instanceCI.ppEnabledLayerNames = layers.data();
        instanceCI.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        instanceCI.ppEnabledExtensionNames = extensions.data();

        CheckVulkanResult(vkCreateInstance(&instanceCI, nullptr, &m_Instance), 
            "Vulkan Instance was not created");
    }

    void VulkanDevice::PickPhysicalDevice(VkSurfaceKHR surface_)
    {
        uint32_t physicalDeviceCount{ 0 };
        vkEnumeratePhysicalDevices(m_Instance, &physicalDeviceCount, nullptr);

        std::vector<VkPhysicalDevice> physicalDevices{ physicalDeviceCount };
        vkEnumeratePhysicalDevices(m_Instance, &physicalDeviceCount, physicalDevices.data());

        uint32_t rateScore{0};
        for (auto&& phDevice : physicalDevices) 
        {
            const uint32_t score = RateDeviceSuitability(phDevice);
            if (rateScore < score) 
            {
                rateScore = score;
                m_PhysicalDevice = phDevice;
            }
        }

        if (!m_PhysicalDevice )
        {
            throw std::runtime_error("Physical Device was not picked");
        }

        if (!PickQueueIndecies(m_PhysicalDevice, surface_)) 
        {
            throw std::runtime_error("Graphics Family Queue was not found");
        }

        // TODO: Get max MSAA
    }

    void VulkanDevice::CreateLogicalDevice()
    {
        // TODO: Support transfer and compute
        std::unordered_set<uint32_t> uniqueIndices{ 
            m_QueueIndices.graphicsQueueIndex, m_QueueIndices.presentQueueIndex };

        std::vector<float> queuePriorities{ 1.f };
        std::vector<VkDeviceQueueCreateInfo> queueCIs;
        queueCIs.reserve(uniqueIndices.size());

        VkDeviceQueueCreateInfo queueCI{};
        queueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCI.queueCount = 1;
        queueCI.pQueuePriorities = queuePriorities.data();
        for (auto&& index : uniqueIndices) 
        {
            queueCI.queueFamilyIndex = index;
            queueCIs.emplace_back(queueCI);
        }

        std::vector<const char*> layers{};
        std::vector<const char*> extensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        VkPhysicalDeviceFeatures features{};
        features.samplerAnisotropy = VK_TRUE;
        features.sampleRateShading = VK_TRUE;

        VkDeviceCreateInfo deviceCI{};
        deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCI.pNext = nullptr;
        deviceCI.flags = 0;
        deviceCI.queueCreateInfoCount = static_cast<uint32_t>(queueCIs.size());
        deviceCI.pQueueCreateInfos = queueCIs.data();
        deviceCI.enabledLayerCount = static_cast<uint32_t>(layers.size());
        deviceCI.ppEnabledLayerNames = layers.data();
        deviceCI.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        deviceCI.ppEnabledExtensionNames = extensions.data();
        deviceCI.pEnabledFeatures = &features;

        CheckVulkanResult(
            vkCreateDevice(m_PhysicalDevice, &deviceCI, nullptr, &m_Device),
            "Device was not created");
    }

    uint32_t VulkanDevice::RateDeviceSuitability(VkPhysicalDevice phDevice_) 
    {
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(phDevice_, &features);

        if (features.geometryShader != VK_TRUE || 
            features.samplerAnisotropy != VK_TRUE) 
        {
            return 0;
        }

        // Check other features ...

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(phDevice_, &properties);

        uint32_t score{0};
        bool suitable = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        score += 100 * suitable;

#ifndef NDEBUG
        suitable = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU;
        score += 10 * suitable;
#endif // NDEBUG

        // Check other properties ...

        uint32_t availableExtensionsCount{};
        vkEnumerateDeviceExtensionProperties(phDevice_, 
            nullptr, &availableExtensionsCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(availableExtensionsCount);
        vkEnumerateDeviceExtensionProperties(phDevice_, nullptr, 
            &availableExtensionsCount, availableExtensions.data());

        const std::vector<const char*> deviceExtensions {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME };

        std::unordered_set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (auto&& extension : availableExtensions) 
        {
            requiredExtensions.erase(extension.extensionName);
            if (requiredExtensions.empty()) 
            {
                break;
            }
        }

        if (!requiredExtensions.empty()) 
        {
            return 0;
        }
    
        return score;
    }

    bool VulkanDevice::PickQueueIndecies(VkPhysicalDevice phDevice_, VkSurfaceKHR surface_)
    {
        uint32_t queueFamilyPropertiesCount{0};
        vkGetPhysicalDeviceQueueFamilyProperties(phDevice_, &queueFamilyPropertiesCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertiesCount);
        vkGetPhysicalDeviceQueueFamilyProperties(phDevice_, &queueFamilyPropertiesCount, queueFamilyProperties.data());
    
        uint8_t min_score{UINT8_MAX};
        for(uint32_t i{0}, n = static_cast<uint32_t>(queueFamilyProperties.size()); i < n; ++i) 
        {
            uint8_t score{0};

            // Graphics queue
            if (m_QueueIndices.graphicsQueueIndex == UINT32_MAX && 
                queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) 
            {
                m_QueueIndices.graphicsQueueIndex = i;
                ++score;
            }

            // Present queu
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(phDevice_, i, surface_, &presentSupport);
            if (m_QueueIndices.presentQueueIndex == UINT32_MAX && 
                presentSupport == VK_TRUE) 
            {
                m_QueueIndices.presentQueueIndex = i;
                ++score;
            }

            // Compute queue
            if (queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) 
            {
                m_QueueIndices.computeQueueIndex = i;
                ++score;
            }

            // Transfer queue
            if (min_score > score && queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) 
            {
                min_score = score;
                m_QueueIndices.transferQueueIndex = i;
            }
        }

        return !(m_QueueIndices.graphicsQueueIndex == UINT32_MAX);
    }
}
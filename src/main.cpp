#include <stdio.h>

#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <vector>
#include <string>
#include <algorithm>

static char const* AppName = "Victory Application";
static char const* EngineName = "Victory Engine";

void CollectLayers(std::vector<const char*>& layers_) {
#ifdef _DEBUG
    layers_.emplace_back("VK_LAYER_KHRONOS_validation");
#endif
}

void CollectExtensions(std::vector<const char*>& extensions_) {
#ifdef _DEBUG
    extensions_.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    extensions_.reserve(glfwExtensionCount + 1);
    for (uint32_t i{0}; i < glfwExtensionCount; ++i) {
        extensions_.emplace_back(glfwExtensions[i]);
    }
}

vk::Instance CreateInstance() {
    vk::ApplicationInfo applicationInfo{
        AppName, 
        VK_MAKE_VERSION(1,0,0), 
        EngineName, 
        VK_MAKE_VERSION(1,0,0), 
        VK_API_VERSION_1_0
    };

    std::vector<const char*> layers;
    CollectLayers(layers);

    std::vector<const char*> extensions;
    CollectExtensions(extensions);

    vk::InstanceCreateInfo instanceCreateInfo{{}, &applicationInfo, layers, extensions};

    return vk::createInstance(instanceCreateInfo);
}

#ifdef _DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback( 
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    printf("Validation layer: %s", pCallbackData->pMessage);
    return VK_FALSE;
}

vk::DebugUtilsMessengerEXT RegisterDebugUtilsMessenger(const vk::Instance& instance_)  {
    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
    debugCreateInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    debugCreateInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
    debugCreateInfo.pfnUserCallback = DebugReportCallback;
    
    vk::DispatchLoaderDynamic dynamicLoader(instance_, vkGetInstanceProcAddr);
    return instance_.createDebugUtilsMessengerEXT(debugCreateInfo, nullptr, dynamicLoader);
}
#endif

bool supportsPresentation(VkPhysicalDevice physicalDevice, uint32_t familyIndex) {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
	return vkGetPhysicalDeviceWin32PresentationSupportKHR(physicalDevice, familyIndex);
#else
	return true;
#endif
}

vk::PhysicalDevice& PickPhysicalDevice(std::vector<vk::PhysicalDevice>& physicalDevices_, uint32_t& graphicsFamilyIndex) {
    VkPhysicalDevice discrete = 0;
    VkPhysicalDevice fallback = 0;

    auto&& found = std::find_if(physicalDevices_.begin(), physicalDevices_.end(), 
        [&graphicsFamilyIndex](const vk::PhysicalDevice& physicalDevice) {
            const vk::PhysicalDeviceProperties deviceProperties = physicalDevice.getProperties();
            if (deviceProperties.deviceType != vk::PhysicalDeviceType::eDiscreteGpu) {
                return false;
            }

            //const vk::PhysicalDeviceFeatures deviceFeatures = physicalDevice.getFeatures();
            //const vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();
            const std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
            for (size_t i{0}, n = queueFamilyProperties.size(); i < n; ++i) {
                if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) && 
                    (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eCompute)) {
                    graphicsFamilyIndex = static_cast<uint32_t>(i);
                    break;
                }
                return false;
            }

            if (!supportsPresentation(physicalDevice, graphicsFamilyIndex)) {
                return false;
            }

            return true;
        }
    );

    if (found == physicalDevices_.end()) {
        throw std::runtime_error("Suitable GPU was no found");
    }

    return *found;
}

vk::Device CreateDevice(vk::Instance& instance_, vk::PhysicalDevice& physicalDevice_, uint32_t graphicalFamilyIndex_) {
    float queuePriorities[] = { 1.0f };

    vk::DeviceQueueCreateInfo queueCreateInfo{
        {},
        graphicalFamilyIndex_,
        1,
        queuePriorities,
    };

    std::vector<const char*> extensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    vk::DeviceCreateInfo deviceCreateInfo{
        {},
        queueCreateInfo,
        {},
        extensions
    };

    return physicalDevice_.createDevice(deviceCreateInfo);
}

vk::SurfaceKHR CreateSurface(vk::Instance& instance_, GLFWwindow* window_, vk::PhysicalDevice& physicalDevice_, uint32_t graphicsFamilyIndex_) {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(static_cast<VkInstance>(instance_), window_, nullptr, &surface) != VK_SUCCESS) {
        glfwDestroyWindow(window_);
        glfwTerminate();
        throw std::runtime_error("glfw surface was not created");
    }

    vk::SurfaceKHR vksurface = vk::SurfaceKHR(surface);
    if (!physicalDevice_.getSurfaceSupportKHR(graphicsFamilyIndex_, vksurface)) {
        throw std::runtime_error("Present is not supported");
    }

    return vksurface;
}

vk::Format GetSwapchainFormat(vk::PhysicalDevice& physicalDevice_, vk::SurfaceKHR& surface_) {
    std::vector<vk::SurfaceFormatKHR> surfaceFormats = physicalDevice_.getSurfaceFormatsKHR(surface_);
    if (!surfaceFormats.size()) {
        throw std::runtime_error("Surface Formats were not found");
    }

    if (surfaceFormats.size() == 1 && surfaceFormats[0].format == vk::Format::eUndefined) {
        return vk::Format::eR8G8B8A8Unorm;
    }

    auto&& found = std::find_if(surfaceFormats.begin(), surfaceFormats.end(),
        [](vk::SurfaceFormatKHR& surfaceFormat) {
            return surfaceFormat.format == vk::Format::eR8G8B8A8Unorm || surfaceFormat.format == vk::Format::eB8G8R8A8Unorm;
        }
    );

    if (found != surfaceFormats.end()) {
        return (*found).format;
    }

    return surfaceFormats[0].format;
}

vk::SwapchainKHR CreateSwapchain(vk::Device& device_, vk::SurfaceKHR& surface_, vk::SurfaceCapabilitiesKHR& surfaceCapabilities_, 
    uint32_t graphicsFamilyIndex, vk::Format& format_, uint32_t width_, uint32_t height_, vk::SwapchainKHR& oldSwapchain_) {
    vk::CompositeAlphaFlagBitsKHR surfaceComposite =
        (surfaceCapabilities_.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eOpaque)
        ? vk::CompositeAlphaFlagBitsKHR::eOpaque
        : (surfaceCapabilities_.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied)
        ? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied
        : (surfaceCapabilities_.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied)
        ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied
        : vk::CompositeAlphaFlagBitsKHR::eInherit;

    vk::SwapchainCreateInfoKHR swapchainCreateInfo{
        {},
        surface_,
        surfaceCapabilities_.minImageCount + 1,
        format_,
        vk::ColorSpaceKHR::eSrgbNonlinear,
        vk::Extent2D(width_, height_),
        1,
        vk::ImageUsageFlagBits::eColorAttachment,
        vk::SharingMode::eExclusive,
        1,
        &graphicsFamilyIndex,
        vk::SurfaceTransformFlagBitsKHR::eIdentity,
        surfaceComposite,
        vk::PresentModeKHR::eFifo,
        VK_FALSE,
        oldSwapchain_
    };

    return device_.createSwapchainKHR(swapchainCreateInfo);
}

vk::Semaphore CreateVkSemaphore(vk::Device& device_) {
    vk::SemaphoreCreateInfo semaphoreCreateInfo{};
    return device_.createSemaphore(semaphoreCreateInfo, nullptr);
}

vk::RenderPass CreateRenderPass(vk::Device& device_, vk::Format& format_) {
    std::vector<vk::AttachmentDescription> attachments{
        {
            {}, format_,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eColorAttachmentOptimal
        }
    }; 

    vk::AttachmentReference colorAttachments{0, vk::ImageLayout::eColorAttachmentOptimal};

    vk::SubpassDescription subpass{};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachments;

    vk::RenderPassCreateInfo renderPassCreateInfo{
        {},
        attachments,
        subpass
    };

    return device_.createRenderPass(renderPassCreateInfo);
}

vk::ShaderModule LoadShader(vk::Device& device_, std::string&& path_)
{
    FILE* file;
    if (fopen_s(&file, path_.c_str(), "rb")) {
        throw std::runtime_error("Failed to open shader file.");
    }
    fseek(file, 0, SEEK_END);
    uint32_t length = static_cast<uint32_t>(ftell(file));
    fseek(file, 0, SEEK_SET);

    char*buffer = new char[length];

    size_t rc = fread(buffer, 1, length, file);
    fclose(file);

    vk::ShaderModuleCreateInfo shaderModuleCreateInfo{
        {},
        length,
        reinterpret_cast<const uint32_t*>(buffer)
    };

    vk::ShaderModule shader = device_.createShaderModule(shaderModuleCreateInfo); 
    delete[] buffer;

    return shader;
}

vk::PipelineLayout CreatePipelineLayout(vk::Device& device_) {
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    return device_.createPipelineLayout(pipelineLayoutCreateInfo);
}

vk::Pipeline CreateGraphicsPipline(vk::Device& device_, vk::PipelineCache& pipelineCache_, vk::RenderPass& renderPass_, 
    vk::ShaderModule& vs_, vk::ShaderModule& fs_, vk::PipelineLayout& pipelineLayout_) {
    std::vector<vk::PipelineShaderStageCreateInfo> stageCreateInfo = {
        {
            {},
            vk::ShaderStageFlagBits::eVertex,
            vs_,
            "main"
        },
        {
            {},
            vk::ShaderStageFlagBits::eFragment,
            fs_,
            "main"
        }
    };

    vk::PipelineVertexInputStateCreateInfo vertexInputCreateInfo{};

    vk::PipelineInputAssemblyStateCreateInfo inputAssemlyCreateInfo{
        {},
        vk::PrimitiveTopology::eTriangleList
    };

    vk::PipelineTessellationStateCreateInfo tessellationStateCreateInfo{};

    vk::PipelineViewportStateCreateInfo viewportStateCreateInfo{};
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.scissorCount = 1;

    vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
    rasterizationStateCreateInfo.lineWidth = 1.f;

    vk::PipelineMultisampleStateCreateInfo mulisampleStateCreateInfo{};
    mulisampleStateCreateInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{};

    vk::PipelineColorBlendAttachmentState colorAttachmentState{};
	colorAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
    colorBlendStateCreateInfo.attachmentCount = 1;
    colorBlendStateCreateInfo.pAttachments = &colorAttachmentState;

    std::vector<vk::DynamicState> dynamicStates{vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo{
        {},
        dynamicStates
    };

    vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo{
        {},
        stageCreateInfo,
        &vertexInputCreateInfo,
        &inputAssemlyCreateInfo,
        &tessellationStateCreateInfo,
        &viewportStateCreateInfo,
        &rasterizationStateCreateInfo,
        &mulisampleStateCreateInfo,
        &depthStencilStateCreateInfo,
        &colorBlendStateCreateInfo,
        &dynamicStateCreateInfo,
        pipelineLayout_,
        renderPass_
    };

    return device_.createGraphicsPipeline(pipelineCache_, graphicsPipelineCreateInfo).value;
}

struct Swapchain {
	vk::SwapchainKHR swapchain;

	std::vector<vk::Image> images;
	std::vector<vk::ImageView> imageViews;
	std::vector<vk::Framebuffer> frameBuffers;

	uint32_t width, height;
	uint32_t imageCount;
};

vk::ImageView createImageView(vk::Device device_, vk::Image image_, vk::Format format_) {
    vk::ImageViewCreateInfo imageViewCreateInfo{
        {},
        image_,
        vk::ImageViewType::e2D,
        format_,
        vk::ComponentMapping(),
        vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
    };

	return device_.createImageView(imageViewCreateInfo);
}

vk::Framebuffer createFramebuffer(vk::Device device_, vk::RenderPass renderPass_, vk::ImageView imageView_, uint32_t width_, uint32_t height_) {

    vk::FramebufferCreateInfo framebufferCreateInfo{
        {},
        renderPass_,
        1,
        &imageView_,
        width_,
        height_,
        1
    };

	return device_.createFramebuffer(framebufferCreateInfo);
}

void CreateSwapchain(Swapchain& result_, vk::PhysicalDevice& physicalDevice_, vk::Device& device_, vk::SurfaceKHR& surface_, uint32_t graphicsFamilyIndex_, 
    vk::Format& format_, uint32_t width_, uint32_t height_, vk::RenderPass& renderPass_, vk::SwapchainKHR oldSwapchain_ = nullptr) {

    vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice_.getSurfaceCapabilitiesKHR(surface_);

    vk::SwapchainKHR swapchain = CreateSwapchain(device_, surface_, surfaceCapabilities, graphicsFamilyIndex_, format_, width_, height_, oldSwapchain_);

    std::vector<vk::Image> images = device_.getSwapchainImagesKHR(swapchain);
    uint32_t imageCount = static_cast<uint32_t>(images.size());

    std::vector<vk::ImageView> imageViews;
    imageViews.reserve(imageCount);
    for (uint32_t i = 0; i < imageCount; ++i)
	{
		imageViews.emplace_back(createImageView(device_, images[i], format_));
	}

    std::vector<vk::Framebuffer> framebuffers;
    framebuffers.reserve(imageCount);
	for (uint32_t i = 0; i < imageCount; ++i)
	{
		framebuffers.emplace_back(createFramebuffer(device_, renderPass_, imageViews[i], width_, height_));
	}

    result_ = { 
        swapchain,
        images,
        imageViews,
        framebuffers,
        width_,
        height_,
        imageCount
    };
}

void destroySwapchain(vk::Device& device_, const Swapchain& swapchain_) {
    for(const vk::Framebuffer& frameBuffer : swapchain_.frameBuffers) {
        device_.destroyFramebuffer(frameBuffer);
    }

    for(const vk::ImageView& imageView : swapchain_.imageViews) {
        device_.destroyImageView(imageView);
    }

    device_.destroySwapchainKHR(swapchain_.swapchain);
}

vk::CommandPool CreateCommandPool(vk::Device& device_, uint32_t graphicsFamilyIndex) {
    vk::CommandPoolCreateInfo commandPoolCreateInfo{
        vk::CommandPoolCreateFlagBits::eTransient,
        graphicsFamilyIndex
    };

    return device_.createCommandPool(commandPoolCreateInfo);
}

std::vector<vk::CommandBuffer> CreateCommandBuffer(vk::Device& device_, vk::CommandPool& commandPool_) {
    vk::CommandBufferAllocateInfo commandBufferAllocateInfo{
        commandPool_,
        vk::CommandBufferLevel::ePrimary,
        1
    };

    return device_.allocateCommandBuffers(commandBufferAllocateInfo);
}

void ResizeSwapchain(Swapchain& result_, vk::PhysicalDevice& physicalDevice_, vk::Device& device_, vk::SurfaceKHR surface_,
    uint32_t graphicsFamilyIndex_, vk::Format& format_, vk::RenderPass& renderPass_) {
    vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice_.getSurfaceCapabilitiesKHR(surface_);

    uint32_t newWidth = surfaceCapabilities.currentExtent.width;
    uint32_t newHeigth = surfaceCapabilities.currentExtent.height;

    if (result_.width == newWidth && result_.height == newHeigth) {
        return;
    }

    Swapchain old = result_;

    CreateSwapchain(result_, physicalDevice_, device_, surface_, graphicsFamilyIndex_, format_, newWidth, newHeigth, renderPass_, old.swapchain);

    device_.waitIdle();
    destroySwapchain(device_, old);
}

vk::ImageMemoryBarrier ImageBarrier(vk::Image& image, vk::AccessFlagBits srcAccessMask, 
    vk::AccessFlagBits dstAccessMask, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) {
    return vk::ImageMemoryBarrier{
        srcAccessMask,
        dstAccessMask,
        oldLayout,
        newLayout,
        vk::QueueFamilyIgnored,
        vk::QueueFamilyIgnored,
        image,
        vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, vk::RemainingMipLevels, 0, vk::RemainingArrayLayers}
    };
}

int main() {
    if (!glfwInit()) {
        throw std::runtime_error("GLFW was not inited");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* window = glfwCreateWindow(1024, 780, AppName, 0, 0);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("GLFW window was not created");
    }

    vk::Instance instance = CreateInstance();

#ifdef _DEBUG
    vk::DebugUtilsMessengerEXT debugUtilsMessenger = RegisterDebugUtilsMessenger(instance);
#endif

    uint32_t graphicsFamilyIndex{0};
    std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
    vk::PhysicalDevice physicalDevice = PickPhysicalDevice(physicalDevices, graphicsFamilyIndex);

    vk::Device device = CreateDevice(instance, physicalDevice, graphicsFamilyIndex);

    vk::SurfaceKHR surface = CreateSurface(instance, window, physicalDevice, graphicsFamilyIndex);

    int windowWidth{0}, windowHeight{0};
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    vk::Format swapchainFormat = GetSwapchainFormat(physicalDevice, surface);

    vk::Semaphore acquireSemaphore = CreateVkSemaphore(device);
    vk::Semaphore releaseSemaphore = CreateVkSemaphore(device);

    vk::Queue queue = device.getQueue(graphicsFamilyIndex, 0);
    vk::RenderPass renderPass = CreateRenderPass(device, swapchainFormat);

    vk::ShaderModule triangleVS = LoadShader(device, "../../shaders/triangle.vert.spv");
    vk::ShaderModule triangleFS = LoadShader(device, "../../shaders/triangle.frag.spv");

    vk::PipelineCache piplineCash{};

    vk::PipelineLayout triangleLayout = CreatePipelineLayout(device);

    vk::Pipeline trianglePipline = CreateGraphicsPipline(device, piplineCash, renderPass, triangleVS, triangleFS, triangleLayout);

    Swapchain swapchain;
    CreateSwapchain(swapchain, physicalDevice, device, surface, graphicsFamilyIndex, swapchainFormat, 
        static_cast<uint32_t>(windowWidth), static_cast<uint32_t>(windowHeight), renderPass);

    vk::CommandPool commandPool = CreateCommandPool(device, graphicsFamilyIndex);

    std::vector<vk::CommandBuffer> commandBuffers = CreateCommandBuffer(device, commandPool);
    vk::CommandBuffer commandBuffer = commandBuffers[0];

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ResizeSwapchain(swapchain, physicalDevice, device, surface, graphicsFamilyIndex, swapchainFormat, renderPass);
        uint32_t imageIndex = device.acquireNextImageKHR(swapchain.swapchain, ~0ull, acquireSemaphore, VK_NULL_HANDLE).value;

        device.resetCommandPool(commandPool);

        vk::CommandBufferBeginInfo commandBufferBeginInfo{
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit
        };

        commandBuffer.begin(commandBufferBeginInfo);

        vk::ImageMemoryBarrier renderBeginBarrier = ImageBarrier(swapchain.images[imageIndex], vk::AccessFlagBits::eNone, 
            vk::AccessFlagBits::eColorAttachmentWrite, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);
        commandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eColorAttachmentOutput, 
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::DependencyFlagBits::eByRegion, 
            0, nullptr, 0, nullptr, 1, 
            &renderBeginBarrier
        );

        vk::ClearColorValue color{48.f / 255.f, 10.f / 255.f, 36.f / 255.f, 1.f};
        std::vector<vk::ClearValue> clearColors{color};


        vk::RenderPassBeginInfo passBeginInfo{
            renderPass,
            swapchain.frameBuffers[imageIndex],
            {{0,0},{swapchain.width,swapchain.height}},
            clearColors
        };

        commandBuffer.beginRenderPass(passBeginInfo, vk::SubpassContents::eInline);

        vk::Viewport viewport{
            0.f,
            static_cast<float>(swapchain.height),
            static_cast<float>(swapchain.width),
            -static_cast<float>(swapchain.height),
            0.f,
            1.f
        };

        vk::Rect2D scissor{
            {
                0,
                0
            },
            {
                swapchain.width,
                swapchain.height
            }
        };

        commandBuffer.setViewport(0, 1, &viewport);
        commandBuffer.setScissor(0, 1, &scissor);

        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, trianglePipline);
        commandBuffer.draw(3, 1, 0, 0);

        commandBuffer.endRenderPass();

        vk::ImageMemoryBarrier renderEndBarrier = ImageBarrier(swapchain.images[imageIndex], vk::AccessFlagBits::eColorAttachmentWrite, 
            vk::AccessFlagBits::eNone, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR);
        commandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eColorAttachmentOutput, 
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::DependencyFlagBits::eByRegion,
            0, nullptr, 0, nullptr, 1,
            &renderEndBarrier
        );

        commandBuffer.end();

        vk::PipelineStageFlags submitStageMask{vk::PipelineStageFlagBits::eColorAttachmentOutput};

        vk::SubmitInfo submitInfo{
            1,
            &acquireSemaphore,
            &submitStageMask,
            1,
            &commandBuffer,
            1,
            &releaseSemaphore
        };

        if (queue.submit(1, &submitInfo, VK_NULL_HANDLE) != vk::Result::eSuccess) {
            printf("smth wrong");
        }

        vk::PresentInfoKHR presentInfo{
            1,
            &releaseSemaphore,
            1,
            &swapchain.swapchain,
            &imageIndex
        };

        if (queue.presentKHR(&presentInfo) != vk::Result::eSuccess) {
            printf("smth wrong");
        }

        device.waitIdle();

        glfwWaitEvents();
    }
    
    device.waitIdle();

    device.freeCommandBuffers(commandPool, commandBuffers);
    device.destroyCommandPool(commandPool);
    destroySwapchain(device, swapchain);
    device.destroyPipeline(trianglePipline);
    device.destroyPipelineLayout(triangleLayout);
    device.destroyShaderModule(triangleFS);
    device.destroyShaderModule(triangleVS);
    device.destroyRenderPass(renderPass);
    device.destroySemaphore(acquireSemaphore);
    device.destroySemaphore(releaseSemaphore);
    instance.destroySurfaceKHR(surface);
    device.destroy();
#ifdef _DEBUG
    vk::DispatchLoaderDynamic dynamicLoader(instance, vkGetInstanceProcAddr);
    instance.destroyDebugUtilsMessengerEXT(debugUtilsMessenger, nullptr, dynamicLoader);
#endif
    instance.destroy();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
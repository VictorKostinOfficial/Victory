#include "Application.h"

#include <stdexcept>
// #include <iostream>

// #include <imgui.h>
// #include <imgui_impl_glfw.h>
// #include <imgui_impl_vulkan.h>

// #include <vulkan/vulkan.hpp>
// #include <GLFW/glfw3.h>
// #include <GLFW/glfw3native.h>

#include "renderer/GraphicsRenderer.h"

// static char const* AppName{ "Victory Application" };
// static char const* EngineName{ "Victory Engine" };

// struct Swapchain {
// 	vk::SwapchainKHR swapchain;

// 	std::vector<vk::Image> images;
// 	std::vector<vk::ImageView> imageViews;
// 	std::vector<vk::Framebuffer> frameBuffers;

// 	uint32_t width, height;
// 	uint32_t imageCount;
// };

// static VkAllocationCallbacks* g_Allocator{ VK_NULL_HANDLE };
// static vk::Instance g_Instance{ VK_NULL_HANDLE };
// static vk::PhysicalDevice g_PhysicalDevice{ VK_NULL_HANDLE };
// static vk::Device g_Device{ VK_NULL_HANDLE };

// static vk::SurfaceKHR g_Surface{ VK_NULL_HANDLE };
// static vk::Format g_SurfaceFormat{ vk::Format::eUndefined };
// static vk::RenderPass g_RenderPass{ VK_NULL_HANDLE };

// static Swapchain g_Swapchain{ VK_NULL_HANDLE };
// static vk::CommandPool g_CommandPool{ VK_NULL_HANDLE };

// static vk::Queue g_Queue{ VK_NULL_HANDLE};
// static vk::PipelineCache g_PipelineCache{ VK_NULL_HANDLE };
// static vk::PipelineLayout g_PipelineLayout{ VK_NULL_HANDLE };
// static vk::Pipeline g_Pipeline{ VK_NULL_HANDLE };
// static vk::DescriptorPool g_DescriptorPool{ VK_NULL_HANDLE };

// static uint32_t g_QueueFamilyIndex{0};
// static uint32_t g_MinImageCount{2};
// static bool g_NeedSwapcahinRebuild{false};


// #ifdef _DEBUG
// static vk::DebugUtilsMessengerEXT g_DebugMessenger{ VK_NULL_HANDLE };

// VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback( 
//     VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
//     VkDebugUtilsMessageTypeFlagsEXT messageType,
//     const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
//     void* pUserData) {
//     std::cout << "\nValidation layer: " << pCallbackData->pMessage << std::endl;
//     return VK_FALSE;
// }

// void RegisterDebugUtilsMessenger()  {
//     vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
//     debugCreateInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
//                                 vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
//                                 vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
//     debugCreateInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
//                             vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
//                             vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
//     debugCreateInfo.pfnUserCallback = DebugReportCallback;
    
//     vk::DispatchLoaderDynamic dynamicLoader(g_Instance, vkGetInstanceProcAddr);
//     g_DebugMessenger = g_Instance.createDebugUtilsMessengerEXT(debugCreateInfo, 
//         nullptr, dynamicLoader);
// }
// #endif

// void check_vk_result(VkResult err)
// {
//     if (err == 0)
//         return;
//     fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
//     if (err < 0)
//         abort();
// }

// void CollectLayers(std::vector<const char*>& layers_) {
// #ifdef _DEBUG
//     layers_.emplace_back("VK_LAYER_KHRONOS_validation");
// #endif
// }

// void CollectExtensions(std::vector<const char*>& extensions_) {
// #ifdef _DEBUG
//     extensions_.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
// #endif

//     uint32_t glfwExtensionCount = 0;
//     const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
//     extensions_.reserve(glfwExtensionCount + 1);
//     for (uint32_t i{0}; i < glfwExtensionCount; ++i) {
//         extensions_.emplace_back(glfwExtensions[i]);
//     }
// }

// void CreateInstance() {

//     vk::ApplicationInfo applicationInfo{
//         AppName, 
//         VK_MAKE_VERSION(1,0,0), 
//         EngineName, 
//         VK_MAKE_VERSION(1,0,0), 
//         VK_API_VERSION_1_0
//     };

//     std::vector<const char*> layers;
//     CollectLayers(layers);

//     std::vector<const char*> extensions;
//     CollectExtensions(extensions);

//     vk::InstanceCreateInfo instanceCreateInfo{
// 		{}, &applicationInfo, 
// 		layers, 
// 		extensions
// 	};

//     g_Instance = vk::createInstance(instanceCreateInfo);

// #ifdef _DEBUG
// 	RegisterDebugUtilsMessenger();
// #endif
// }

// void PickPhysicalDevice() {

// 	std::vector<vk::PhysicalDevice> physicalDevices = g_Instance.enumeratePhysicalDevices();
//     auto&& found = std::find_if(physicalDevices.begin(), physicalDevices.end(), 
//         [](const vk::PhysicalDevice& physicalDevice) {

//             const vk::PhysicalDeviceProperties deviceProperties = physicalDevice.getProperties();
//             if (deviceProperties.deviceType != vk::PhysicalDeviceType::eDiscreteGpu) {
//                 return false;
//             }

//             const std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
//             for (size_t i{0}, n = queueFamilyProperties.size(); i < n; ++i) {
//                 if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
//                     g_QueueFamilyIndex = static_cast<uint32_t>(i);
//                     break;
//                 }
//             }

// 			// TODO: Check presentation support

//             return true;
//         }
//     );

//     if (found == physicalDevices.end()) {
//         throw std::runtime_error("Suitable GPU was no found");
//     }

//     g_PhysicalDevice = *found;
// }

// void CreateDevice() {

// 	// TODO: what am i multiple queues?
//     std::vector<float> queuePriorities{ 1.0f };
//     vk::DeviceQueueCreateInfo queueCreateInfo{
//         {}, g_QueueFamilyIndex,
// 		queuePriorities
//     };

//     std::vector<const char*> layers{};
//     std::vector<const char*> extensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
//     vk::DeviceCreateInfo deviceCreateInfo{
//         {}, queueCreateInfo,
// 		layers,
//         extensions
//     };

//     g_Device = g_PhysicalDevice.createDevice(deviceCreateInfo);
//     g_Queue = g_Device.getQueue(g_QueueFamilyIndex, 0);
// }

// void CreateDescriptorPool() {
//     std::vector<vk::DescriptorPoolSize> pool_sizes =
//         {
//             { vk::DescriptorType::eSampler, 1000 },
//             { vk::DescriptorType::eCombinedImageSampler, 1000 },
//             { vk::DescriptorType::eSampledImage, 1000 },
//             { vk::DescriptorType::eStorageImage, 1000 },
//             { vk::DescriptorType::eUniformTexelBuffer, 1000 },
//             { vk::DescriptorType::eStorageTexelBuffer, 1000 },
//             { vk::DescriptorType::eUniformBuffer, 1000 },
//             { vk::DescriptorType::eStorageBuffer, 1000 },
//             { vk::DescriptorType::eUniformBufferDynamic, 1000 },
//             { vk::DescriptorType::eStorageBufferDynamic, 1000 },
//             { vk::DescriptorType::eInputAttachment, 1000 }
//         };

//     vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo{
//         {}, 1000 * static_cast<uint32_t>(pool_sizes.size()),
//         pool_sizes
//     };

//     g_DescriptorPool = g_Device.createDescriptorPool(descriptorPoolCreateInfo);
// }

// void CreateSurface(GLFWwindow* window_) {

//     VkSurfaceKHR surface;
//     if (glfwCreateWindowSurface(static_cast<VkInstance>(g_Instance), window_, nullptr, &surface) != VK_SUCCESS) {
//         glfwDestroyWindow(window_);
//         glfwTerminate();
//         throw std::runtime_error("glfw surface was not created");
//     }

//     g_Surface = static_cast<vk::SurfaceKHR>(surface);
//     if (!g_PhysicalDevice.getSurfaceSupportKHR(g_QueueFamilyIndex, g_Surface)) {
//         throw std::runtime_error("Present is not supported");
//     }
// }

// void GetSwapchainFormat() {

//     std::vector<vk::SurfaceFormatKHR> surfaceFormats = g_PhysicalDevice.getSurfaceFormatsKHR(g_Surface);
//     if (!surfaceFormats.size()) {
//         throw std::runtime_error("Surface Formats were not found");
//     }

//     if (surfaceFormats.size() == 1 && surfaceFormats[0].format == vk::Format::eUndefined) {
//         g_SurfaceFormat = vk::Format::eR8G8B8A8Unorm;
//     }

//     auto&& found = std::find_if(surfaceFormats.begin(), surfaceFormats.end(),
//         [](vk::SurfaceFormatKHR& surfaceFormat) {
//             return surfaceFormat.format == vk::Format::eR8G8B8A8Unorm || 
// 					surfaceFormat.format == vk::Format::eB8G8R8A8Unorm;
//         }
//     );

//     if (found != surfaceFormats.end()) {
//         g_SurfaceFormat = (*found).format;
//     }

//     g_SurfaceFormat = surfaceFormats[0].format;
// }

// void CreateRenderPass() {

// 	GetSwapchainFormat();

//     std::vector<vk::AttachmentDescription> attachments{
//         {
//             {}, g_SurfaceFormat,
//             vk::SampleCountFlagBits::e1,
//             vk::AttachmentLoadOp::eClear,
//             vk::AttachmentStoreOp::eStore,
//             vk::AttachmentLoadOp::eDontCare,
//             vk::AttachmentStoreOp::eDontCare,
//             vk::ImageLayout::eColorAttachmentOptimal,
//             vk::ImageLayout::eColorAttachmentOptimal
//         }
//     }; 

//     vk::AttachmentReference colorAttachments{0, vk::ImageLayout::eColorAttachmentOptimal};
//     vk::SubpassDescription subpass{};
//     subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
//     subpass.colorAttachmentCount = 1;
//     subpass.pColorAttachments = &colorAttachments;

//     vk::SubpassDependency dependency{};
//     dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
//     dependency.dstSubpass = 0;
//     dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
//     dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
//     dependency.srcAccessMask = vk::AccessFlagBits::eNone;
//     dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead;

//     vk::RenderPassCreateInfo renderPassCreateInfo{
//         {},
//         attachments,
//         subpass,
//         dependency
//     };

//     g_RenderPass = g_Device.createRenderPass(renderPassCreateInfo);
// }

// vk::ImageView createImageView(vk::Image& image_) {
//     vk::ImageViewCreateInfo imageViewCreateInfo{
//         {},
//         image_,
//         vk::ImageViewType::e2D,
//         g_SurfaceFormat,
//         vk::ComponentMapping(),
//         vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0u, 1u, 0u, 1u},
//     };

// 	return g_Device.createImageView(imageViewCreateInfo);
// }

// vk::Framebuffer createFramebuffer(vk::ImageView& imageView_, uint32_t width_, uint32_t height_) {

//     vk::FramebufferCreateInfo framebufferCreateInfo{
//         {},
//         g_RenderPass,
//         1,
//         &imageView_,
//         width_,
//         height_,
//         1
//     };

// 	return g_Device.createFramebuffer(framebufferCreateInfo);
// }

// void CreateSwapchain(int width_, int height_, vk::SwapchainKHR oldSwapchain_ = { VK_NULL_HANDLE }) {

//     vk::SurfaceCapabilitiesKHR surfaceCapabilities = g_PhysicalDevice.getSurfaceCapabilitiesKHR(g_Surface);

//     vk::CompositeAlphaFlagBitsKHR surfaceComposite =
//         (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eOpaque)
//         ? vk::CompositeAlphaFlagBitsKHR::eOpaque
//         : (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied)
//         ? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied
//         : (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied)
//         ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied
//         : vk::CompositeAlphaFlagBitsKHR::eInherit;
	
// 	g_MinImageCount = g_MinImageCount > surfaceCapabilities.minImageCount + 1 ? 
// 		g_MinImageCount : surfaceCapabilities.minImageCount + 1;
	
//     vk::SwapchainCreateInfoKHR swapchainCreateInfo{
//         {},
//         g_Surface,
// 		g_MinImageCount,
//         g_SurfaceFormat,
//         vk::ColorSpaceKHR::eSrgbNonlinear,
//         vk::Extent2D(width_, height_),
//         1,
//         vk::ImageUsageFlagBits::eColorAttachment,
//         vk::SharingMode::eExclusive,
//         1,
//         &g_QueueFamilyIndex,
//         vk::SurfaceTransformFlagBitsKHR::eIdentity,
//         surfaceComposite,
//         vk::PresentModeKHR::eFifo,
//         VK_FALSE,
//         oldSwapchain_
//     };

// 	g_Swapchain.swapchain = g_Device.createSwapchainKHR(swapchainCreateInfo);

//     std::vector<vk::Image> images = g_Device.getSwapchainImagesKHR(g_Swapchain.swapchain);
//     uint32_t imageCount = static_cast<uint32_t>(images.size());

//     std::vector<vk::ImageView> imageViews;
//     imageViews.reserve(imageCount);
//     for (uint32_t i = 0; i < imageCount; ++i)
// 	{
// 		imageViews.emplace_back(createImageView(images[i]));
// 	}

//     std::vector<vk::Framebuffer> framebuffers;
//     framebuffers.reserve(imageCount);
// 	for (uint32_t i = 0; i < imageCount; ++i)
// 	{
// 		framebuffers.emplace_back(createFramebuffer(imageViews[i], width_, height_));
// 	}

//     g_Swapchain.images = images;
//     g_Swapchain.imageViews = imageViews;
//     g_Swapchain.frameBuffers = framebuffers;
//     g_Swapchain.width = width_;
//     g_Swapchain.height = height_;
//     g_Swapchain.imageCount = imageCount;
// }

// void DestroySwapchain(Swapchain swapchain_) {
//     for(const vk::Framebuffer& frameBuffer : swapchain_.frameBuffers) {
//         g_Device.destroyFramebuffer(frameBuffer);
//     }

//     for(const vk::ImageView& imageView : swapchain_.imageViews) {
//         g_Device.destroyImageView(imageView);
//     }

//     g_Device.destroySwapchainKHR(swapchain_.swapchain);
// }

// void ResizeSwapchain() {

//     vk::SurfaceCapabilitiesKHR surfaceCapabilities = g_PhysicalDevice.getSurfaceCapabilitiesKHR(g_Surface);

//     uint32_t newWidth = surfaceCapabilities.currentExtent.width;
//     uint32_t newHeigth = surfaceCapabilities.currentExtent.height;

//     if (newWidth == g_Swapchain.width && newHeigth == g_Swapchain.height) {
//         return;
//     }

//     Swapchain old = g_Swapchain;
//     CreateSwapchain(newWidth, newHeigth, old.swapchain);

//     g_Device.waitIdle();
//     DestroySwapchain(old);
// }

// vk::ImageMemoryBarrier ImageBarrier(vk::Image& image, vk::AccessFlagBits srcAccessMask, 
//     vk::AccessFlagBits dstAccessMask, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) {
//     return vk::ImageMemoryBarrier{
//         srcAccessMask,
//         dstAccessMask,
//         oldLayout,
//         newLayout,
//         vk::QueueFamilyIgnored,
//         vk::QueueFamilyIgnored,
//         image,
//         vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, vk::RemainingMipLevels, 0, vk::RemainingArrayLayers}
//     };
// }

// void CreateCommandPool() {
//     vk::CommandPoolCreateInfo commandPoolCreateInfo{
//         vk::CommandPoolCreateFlagBits::eProtected,
//         g_QueueFamilyIndex
//     };

//     g_CommandPool = g_Device.createCommandPool(commandPoolCreateInfo);
// }

// std::vector<vk::CommandBuffer> CreateCommandBuffer() {
//     vk::CommandBufferAllocateInfo commandBufferAllocateInfo{
//         g_CommandPool,
//         vk::CommandBufferLevel::ePrimary,
//         1
//     };

//     return g_Device.allocateCommandBuffers(commandBufferAllocateInfo);
// }

// vk::Semaphore CreateVkSemaphore() {
//     vk::SemaphoreCreateInfo semaphoreCreateInfo{};
//     return g_Device.createSemaphore(semaphoreCreateInfo, nullptr);
// }

namespace Victory {
	Application* Application::s_Instance{ nullptr} ;

    Application::Application(const ApplicationSpecification& specification)
		: m_AppSpec(specification)
	{
        if (s_Instance) {
            throw std::runtime_error("Application instance already inited");
        }
		s_Instance = this;
	}

	Application::~Application()
	{
	}

    void Application::Run()
	{
		GraphicsRenderer* GraphicsRenderer = GraphicsRenderer::CreateInstance();
		GraphicsRenderer->CreateInstance();



		GraphicsRenderer->Cleanup();
		GraphicsRenderer::DestroyInstance();
        return;

// 		if (!glfwInit()) {
// 			// TODO: error
// 			return;
// 		}

//     	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
//     	m_Window = glfwCreateWindow(1280, 720, "Victory", nullptr, nullptr);
// 		if (!glfwVulkanSupported()) {
// 			// TODO: error
// 			return;
// 		}

// 		CreateInstance();
// 		PickPhysicalDevice();
// 		CreateDevice();
//         CreateDescriptorPool();

// 		CreateSurface(m_Window);

// 		glfwGetFramebufferSize(m_Window, &m_FrameBufferWidth, &m_FrameBufferHeight);
// 		CreateRenderPass();
// 		CreateSwapchain(m_FrameBufferWidth, m_FrameBufferHeight);



//         // Creating command pool and buffers
//         CreateCommandPool();
//         std::vector<vk::CommandBuffer> commandBuffers = CreateCommandBuffer();
//         vk::CommandBuffer commandBuffer = commandBuffers[0];

// 		IMGUI_CHECKVERSION();
// 		ImGui::CreateContext();
// 		ImGuiIO& io = ImGui::GetIO(); (void)io;
//     	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
//     	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
//     	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
//     	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
//     	//io.ConfigViewportsNoAutoMerge = true;
//     	//io.ConfigViewportsNoTaskBarIcon = true;

//     	ImGui::StyleColorsDark();

// 		ImGuiStyle& style = ImGui::GetStyle();
//     	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
//     	{
//         	style.WindowRounding = 0.0f;
//         	style.Colors[ImGuiCol_WindowBg].w = 1.0f;
//     	}

//     	ImGui_ImplGlfw_InitForVulkan(m_Window, true);
//     	ImGui_ImplVulkan_InitInfo init_info = {};
//     	init_info.Instance = g_Instance;
//     	init_info.PhysicalDevice = g_PhysicalDevice;
//     	init_info.Device = g_Device;
//     	init_info.QueueFamily = g_QueueFamilyIndex;
//     	init_info.Queue = g_Queue;
//     	init_info.PipelineCache = g_PipelineCache;
//     	init_info.DescriptorPool = g_DescriptorPool;  // TODO: configure this
//     	init_info.Subpass = 0;
//     	init_info.MinImageCount = g_MinImageCount;
//     	init_info.ImageCount = g_Swapchain.imageCount;
//     	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
//     	init_info.Allocator = g_Allocator;
//     	init_info.CheckVkResultFn = check_vk_result;
//     	ImGui_ImplVulkan_Init(&init_info, g_RenderPass);

//         // Upload Fonts
// 		{
//             g_Device.resetCommandPool(g_CommandPool);
//             vk::CommandBufferBeginInfo commandBufferBeginInfo{
//                 vk::CommandBufferUsageFlagBits::eOneTimeSubmit
//             };
//             commandBuffer.begin(commandBufferBeginInfo);

// 			ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

//             vk::SubmitInfo submitInfo{};
//             submitInfo.commandBufferCount = 1;
//             submitInfo.pCommandBuffers = &commandBuffer;

//             commandBuffer.end();

//             if (g_Queue.submit(1, &submitInfo, VK_NULL_HANDLE) != vk::Result::eSuccess) {
//                 // TODO: Error
//                 throw std::runtime_error("smth wrong");
//             }

//             g_Device.waitIdle();

// 			ImGui_ImplVulkan_DestroyFontUploadObjects();
// 		}

//         vk::Semaphore acquireSemaphore = CreateVkSemaphore();
//         vk::Semaphore releaseSemaphore = CreateVkSemaphore();

//         vk::ClearColorValue color{48.f / 255.f, 10.f / 255.f, 36.f / 255.f, 1.f};
//         std::vector<vk::ClearValue> clearColors{color};

// 		while (!glfwWindowShouldClose(m_Window)) {

//             glfwPollEvents();

//             ResizeSwapchain();

//             ImGui_ImplVulkan_NewFrame();
//             ImGui_ImplGlfw_NewFrame();
//             ImGui::NewFrame();

// 			// for (auto&& layer : m_LayerStack) {
// 			// 	layer->OnUpdate(1.f);
// 			// }

// 			for (auto&& layer : m_LayerStack) {
// 				layer->OnUIRender();
// 			}

//             ImGui::Render();
//             ImDrawData* main_draw_data = ImGui::GetDrawData();
//             const bool main_is_minimized = 
//                 (main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f);
//             uint32_t imageIndex = g_Device.acquireNextImageKHR(g_Swapchain.swapchain, ~0ull, 
//                 acquireSemaphore, VK_NULL_HANDLE).value;

//             g_Device.resetCommandPool(g_CommandPool);

//             vk::CommandBufferBeginInfo commandBufferBeginInfo{};
//             commandBufferBeginInfo.flags |= vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
//             commandBuffer.begin(commandBufferBeginInfo);

//             vk::ImageMemoryBarrier renderBeginBarrier = ImageBarrier(
//                 g_Swapchain.images[imageIndex], 
//                 vk::AccessFlagBits::eNone, 
//                 vk::AccessFlagBits::eColorAttachmentWrite, 
//                 vk::ImageLayout::eUndefined, 
//                 vk::ImageLayout::eColorAttachmentOptimal
//             );
//             commandBuffer.pipelineBarrier(
//                 vk::PipelineStageFlagBits::eColorAttachmentOutput, 
//                 vk::PipelineStageFlagBits::eColorAttachmentOutput,
//                 vk::DependencyFlagBits::eByRegion, 
//                 0, nullptr, 0, nullptr, 1, 
//                 &renderBeginBarrier
//             );

//             vk::Rect2D scissor{{ 0, 0 }, { g_Swapchain.width, g_Swapchain.height }};

//             vk::RenderPassBeginInfo passBeginInfo{
//                 g_RenderPass,
//                 g_Swapchain.frameBuffers[imageIndex],
//                 scissor, 
//                 clearColors
//             };
            
//             // vk::Viewport viewport{
//             // 0.f,
//             // static_cast<float>(g_Swapchain.height),
//             // static_cast<float>(g_Swapchain.width),
//             // -static_cast<float>(g_Swapchain.height),
//             // 0.f,
//             // 1.f
//             // };

//             commandBuffer.beginRenderPass(passBeginInfo, vk::SubpassContents::eInline);

//             // commandBuffer.setViewport(0, 1, &viewport);
//             // commandBuffer.setScissor(0, 1, &scissor);
//             ImGui_ImplVulkan_RenderDrawData(main_draw_data, commandBuffer);

//             commandBuffer.endRenderPass();

//             vk::ImageMemoryBarrier renderEndBarrier = ImageBarrier(
//                 g_Swapchain.images[imageIndex], 
//                 vk::AccessFlagBits::eColorAttachmentWrite, 
//                 vk::AccessFlagBits::eNone, 
//                 vk::ImageLayout::eColorAttachmentOptimal, 
//                 vk::ImageLayout::ePresentSrcKHR
//             );
//             commandBuffer.pipelineBarrier(
//                 vk::PipelineStageFlagBits::eColorAttachmentOutput, 
//                 vk::PipelineStageFlagBits::eTopOfPipe,
//                 vk::DependencyFlagBits::eByRegion,
//                 0, nullptr, 0, nullptr, 1,
//                 &renderEndBarrier
//             );

//             commandBuffer.end();

//             vk::PipelineStageFlags submitStageMask{vk::PipelineStageFlagBits::eColorAttachmentOutput};
//             vk::SubmitInfo submitInfo{
//             acquireSemaphore,
//             submitStageMask,
//             commandBuffer,
//             releaseSemaphore
//             };

//             if (g_Queue.submit(1, &submitInfo, VK_NULL_HANDLE) != vk::Result::eSuccess) {
//                 printf("smth wrong");
//             }

//             // Update and Render additional Platform Windows
//             if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
//                 ImGui::UpdatePlatformWindows();
//                 ImGui::RenderPlatformWindowsDefault();
//             }

//             vk::PresentInfoKHR presentInfo{
//             1, &releaseSemaphore,
//             1, &g_Swapchain.swapchain,
//             &imageIndex
//             };

//             if (g_Queue.presentKHR(&presentInfo) != vk::Result::eSuccess) {
//                 printf("smth wrong");
//             }

//             g_Device.waitIdle();







//             // if (!main_is_minimized) {
//             //     // TODO: RENDER
//             //     //FrameRender(wd, main_draw_data);
//             // }


//             // // Present Main Platform Window
//             // if (!main_is_minimized) {
//             //     // TODO: PRESENT
//             //     //FramePresent(wd);
//             // }
// 		}

//         g_Device.waitIdle();
//         // ImGui_ImplVulkan_Shutdown();
//         // ImGui_ImplGlfw_Shutdown();
//         // ImGui::DestroyContext();


//         g_Device.destroyDescriptorPool(g_DescriptorPool);
//         g_Device.destroyCommandPool(g_CommandPool);
//         DestroySwapchain(g_Swapchain);
//         g_Device.destroyRenderPass(g_RenderPass);
//         g_Device.destroySemaphore(acquireSemaphore);
//         g_Device.destroySemaphore(releaseSemaphore);
//         g_Instance.destroySurfaceKHR(g_Surface);
//         g_Device.destroy();
// #ifdef _DEBUG
//         vk::DispatchLoaderDynamic dynamicLoader(g_Instance, vkGetInstanceProcAddr);
//         g_Instance.destroyDebugUtilsMessengerEXT(g_DebugMessenger, nullptr, dynamicLoader);
// #endif
//         g_Instance.destroy();

//         glfwDestroyWindow(m_Window);
//         glfwTerminate();
	}
}
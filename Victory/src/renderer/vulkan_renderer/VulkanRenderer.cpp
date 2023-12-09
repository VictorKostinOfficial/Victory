#include <vulkan/vulkan.h>

#include "VulkanRenderer.h"

#include <GLFW/glfw3.h>
#include <stdexcept>
#include <stdio.h>
#include <array>

#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanPipeline.h"
#include "VulkanBuffer.h"
#include "VulkanFrameBuffer.h"
#include "VulkanUtils.h"
#include "VulkanImage.h"
#include "VulkanModel.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

static VkDescriptorPool m_ImGuiPool{VK_NULL_HANDLE};
static VkRenderPass m_ImGuiPass{VK_NULL_HANDLE};
static VulkanFrameBuffer* m_ImGuiFrameBuffer{nullptr};
static VkSampler m_ImGuiSampler{VK_NULL_HANDLE};
static std::vector<VkDescriptorSet> m_Dest;

void DestroyImgui(VkDevice device_, VkDescriptorPool pool_, VkRenderPass pass_) {
    m_ImGuiFrameBuffer->CleanupCommandPool();
    m_ImGuiFrameBuffer->CleanupFrameBuffers();
    vkDestroyRenderPass(device_, pass_, nullptr);
    vkDestroyDescriptorPool(device_, pool_, nullptr);
}

void createImGuiTextureSampler(VkDevice device_, VkSampler &sampler_)
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device_, &samplerInfo, nullptr, &sampler_) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}
}

#define CHK_RESULT(RESULT, MESSAGE) \
    if (RESULT == false) { \
        throw std::runtime_error(MESSAGE); \
    } 

void FrameBufferResizeCallback(GLFWwindow *window_, int width_, int height_) {
    (void)width_;
    (void)height_;
    auto app = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window_));
    app->SetIsResized(true);
}

Renderer* VulkanRenderer::CreateRenderer() {
    return new VulkanRenderer();
}

void VulkanRenderer::Initialize(const char *applicationName_){
    if (!glfwInit()) {
        throw std::runtime_error("GLFW was not created!");
    };

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_Window = glfwCreateWindow(1280, 720, "Victory", nullptr, nullptr);
    if (!m_Window) {
        glfwTerminate();
        throw std::runtime_error("GLFW Window was not created!");
    }

    glfwSetWindowUserPointer(m_Window, this);
    glfwSetFramebufferSizeCallback(m_Window, FrameBufferResizeCallback);

    m_VulkanContext = new VulkanContext();

    CHK_RESULT(m_VulkanContext->CreateInstance(applicationName_), 
        "Insance was not created!");

#ifndef NDEBUG
    CHK_RESULT(m_VulkanContext->RegisterDebugUtilsMessenger(), 
        "Debug messenger was not created!");
#endif

    m_VulkanSwapchain = new VulkanSwapchain(m_VulkanContext);

    CHK_RESULT(m_VulkanSwapchain->CreateSurface(m_Window),
        "Surface was not created!");

    CHK_RESULT(m_VulkanContext->PickPhysicalDevice(m_VulkanSwapchain->GetSurface()), 
        "Physical Device was not picked!");

    CHK_RESULT(m_VulkanContext->CreateLogicalDevice(), 
        "Logical Device was not created!");

    CHK_RESULT(m_VulkanSwapchain->CreateSwapchain(m_Window),
        "Swapchain was not created!");

    CHK_RESULT(m_VulkanSwapchain->CreateImageViews(), 
        "Image views were not created!");

    m_VulkanPipeline = new VulkanPipeline(m_VulkanContext);

    CHK_RESULT(m_VulkanPipeline->CreateRenderPass(m_VulkanSwapchain->GetSurfaceFormat().format),
        "Render pass was not created!");

    CHK_RESULT(m_VulkanPipeline->CreateDescriptorSetLayout(),
        "Descriptor set was not created");
    
    CHK_RESULT(m_VulkanPipeline->CreatePipelineLayout(),
        "Pipeline layout was not created!");

    CHK_RESULT(m_VulkanPipeline->CreatePipeline(),
        "Pipeline was not created!");

    m_VulkanFrameBuffer = new VulkanFrameBuffer(m_VulkanContext, m_VulkanSwapchain, m_VulkanPipeline);

    CHK_RESULT(m_VulkanFrameBuffer->CreateCommandPool(),
        "Command pool was not created!");

    CHK_RESULT(m_VulkanFrameBuffer->CreateDepthResources(),
        "Depth resources were not created");

    CHK_RESULT(m_VulkanFrameBuffer->CreateColorResources(),
        "Color resources were not created");

    CHK_RESULT(m_VulkanFrameBuffer->CreateFrameBuffers(m_VulkanPipeline->GetRenderPass()),
        "Frame buffers were not created!");

    // CHK_RESULT(m_VulkanBuffer->LoadModel(),
    //     "Model was not loaded");
    // CHK_RESULT(m_VulkanBuffer->CreateVertexBuffer(),
    //     "Vertex buffer was not created");
    // CHK_RESULT(m_VulkanBuffer->CreateIndexBuffer(),
    //     "Index buffer was not created");

    m_Models.push_back(VulkanModel(m_VulkanContext, m_VulkanFrameBuffer));
    m_Models[0].LoadModel("viking_room.obj");
    m_Models[0].CreateVertexBuffer();
    m_Models[0].CreateIndexBuffer();
    // m_Models.push_back(VulkanModel(m_VulkanContext, m_VulkanFrameBuffer));
    // m_Models[1].LoadModel("hat_luffy.obj");
    // m_Models[1].CreateVertexBuffer();
    // m_Models[1].CreateIndexBuffer();

    // CHK_RESULT(m_VulkanBuffer->CreateTextureImage(), 
    //     "Texture image was not created");
    // CHK_RESULT(m_VulkanBuffer->CreateTextureImageView(), 
    //     "Texture image view was not created");
    // CHK_RESULT(m_VulkanBuffer->CreateTextureSampler(), 
    //     "Texture sampler view was not created");

    m_Images.push_back(VulkanImage(m_VulkanContext, m_VulkanFrameBuffer));
    CreateImageSettings settings{};
    // settings.Format = VK_FORMAT_R8G8B8A8_SRGB;
    settings.Format = VK_FORMAT_R8G8B8A8_UNORM;
    settings.Tiling = VK_IMAGE_TILING_OPTIMAL;
    settings.Usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    settings.Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    settings.SampleCount = VK_SAMPLE_COUNT_1_BIT;
    m_Images[0].LoadTexture("viking_room.png", settings);
    m_Images[0].CreateImageView(settings.Format, VK_IMAGE_ASPECT_COLOR_BIT);
    m_Images[0].CreateSampler();
    // m_Images.push_back(VulkanImage(m_VulkanContext, m_VulkanFrameBuffer));
    // m_Images[1].LoadTexture("hat_luffy.png", settings);
    // m_Images[1].CreateImageView(settings.Format, VK_IMAGE_ASPECT_COLOR_BIT);
    // m_Images[1].CreateSampler();

    m_Buffers.push_back(VulkanBuffer(m_VulkanContext, m_VulkanPipeline, m_VulkanFrameBuffer, m_VulkanSwapchain));
    // m_Buffers.push_back(VulkanBuffer(m_VulkanContext, m_VulkanPipeline, m_VulkanFrameBuffer, m_VulkanSwapchain));
    for (size_t i{0}, n = m_Buffers.size(); i < n; ++i) {
        CHK_RESULT(m_Buffers[i].CreateUniformBuffers(MAX_FRAMES_IN_FLIGHT),
            "Uniform buffer was not created");

        CHK_RESULT(m_Buffers[i].CreateDescriptorPool(MAX_FRAMES_IN_FLIGHT),
            "Descriptor pool was not created");

        CHK_RESULT(m_Buffers[i].CreateDescriptorSets(MAX_FRAMES_IN_FLIGHT, m_Images[i].GetSampler(), m_Images[i].GetImageView()),
            "Descriptor sets was not created");
    }

    CHK_RESULT(m_VulkanFrameBuffer->CreateCommandBuffer(MAX_FRAMES_IN_FLIGHT),
        "Command buffer was not created!");

    m_AvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_FinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    for (uint32_t i{0}; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        CHK_RESULT(VulkanUtils::CreateSemaphore(m_VulkanContext->GetDevice(), &m_AvailableSemaphores[i]),
            "Available semaphore was not created!");
        CHK_RESULT(VulkanUtils::CreateSemaphore(m_VulkanContext->GetDevice(), &m_FinishedSemaphores[i]),
            "Finished semaphore was not created!");
        CHK_RESULT(VulkanUtils::CreateFence(m_VulkanContext->GetDevice(), &m_InFlightFences[i]),
            "Fence was not created!");
    }

    InitImGui();
}

bool VulkanRenderer::IsRunning() {
    return !glfwWindowShouldClose(m_Window);
}

void VulkanRenderer::PollEvents() {
    glfwPollEvents();
}

bool VulkanRenderer::Resize() {

    VkDevice device = m_VulkanContext->GetDevice();
    CHK_RESULT((vkWaitForFences(device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX) == VK_SUCCESS),
        "Wait for fences = false!");

    VkResult acquireResult = vkAcquireNextImageKHR(device, m_VulkanSwapchain->GetSwapchain(), UINT64_MAX, m_AvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &m_ImageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR || acquireResult == VK_SUBOPTIMAL_KHR || m_IsResized)
    {
        m_IsResized = false;
        RecreateSwapchain();
        vkDestroySemaphore(device, m_AvailableSemaphores[m_CurrentFrame], nullptr);
        VulkanUtils::CreateSemaphore(device, &m_AvailableSemaphores[m_CurrentFrame]);
        return true;
    }
    else if (acquireResult != VK_SUCCESS)
    {
        throw std::runtime_error("Failde to acquire swap chain image!");
    }
    return false;
}

void VulkanRenderer::BeginFrame() {
    printf("\n-------------- BEGIN FRAME -------------");
    for (size_t i{0}, n = m_Buffers.size(); i < n; ++i) {
        m_Buffers[i].UpdateUniformBuffer(m_CurrentFrame);
    }
}

void VulkanRenderer::RecordCommandBuffer() {
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
            ImGui::ShowDemoWindow();
        
            // ImGui::Begin("Viewport");
		    // ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
            // ImGui::Image(m_Dest[m_CurrentFrame], ImVec2{viewportPanelSize.x,viewportPanelSize.y});
            // ImGui::Button("test");
		    // ImGui::End();
        }
        ImGuiIO &io = ImGui::GetIO();
        ImGui::Render();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }

    {
        VkCommandBuffer commandBuffer = m_VulkanFrameBuffer->GetCommandBuffer(m_CurrentFrame);

        VkCommandBufferBeginInfo commandBufferBI{};
        commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &commandBufferBI);

        VkRect2D rect{};
        rect.offset = {0, 0};
        rect.extent = m_VulkanSwapchain->GetExtent();

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{48.f / 255.f, 10.f / 255.f, 36.f / 255.f, 1.f}};
        clearValues[1].depthStencil = {1.f, 0};

        VkRenderPassBeginInfo renderPassBI{};
        renderPassBI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBI.renderPass = m_VulkanPipeline->GetRenderPass();
        renderPassBI.framebuffer = m_VulkanFrameBuffer->GetFrameBuffer(m_ImageIndex);
        renderPassBI.renderArea = rect;
        renderPassBI.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassBI.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);
        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VulkanPipeline->GetPipeline());

            VkViewport viewport{};
            viewport.x = 0.f;
            viewport.y = 0.f;
            viewport.width = static_cast<float>(rect.extent.width);
            viewport.height = static_cast<float>(rect.extent.height);
            viewport.minDepth = 0.f;
            viewport.maxDepth = 1.f;

            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
            vkCmdSetScissor(commandBuffer, 0, 1, &rect);

            for (size_t i{0}, n = m_Models.size(); i < n; ++i) {
                std::vector<VkBuffer> vertexBuffers{m_Models[i].GetVertexBuffer()};
                VkBuffer indexBuffer{m_Models[i].GetIndexBuffer()};
                std::vector<VkDeviceSize> offsets{0};

                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets.data());
                vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VulkanPipeline->GetPipelineLayout(), 
                    0, 1, &m_Buffers[i].GetDescriptorSet(m_CurrentFrame), 0, nullptr);

                vkCmdDrawIndexed(commandBuffer, m_Models[i].GetIndexCount(), 1, 0, 0, 0);

            }
        }
        vkCmdEndRenderPass(commandBuffer);
        vkEndCommandBuffer(commandBuffer);
    }

    {
        VkCommandBuffer ImGuiCommandBuffer = m_ImGuiFrameBuffer->GetCommandBuffer(m_CurrentFrame);
        VkCommandBufferBeginInfo commandBufferBI{};
        commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(ImGuiCommandBuffer, &commandBufferBI);

        VkClearValue clearColor = {0.f, 0.f, 0.f, 0.f};

        VkRenderPassBeginInfo renderPassBI{};
        renderPassBI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBI.renderPass = m_ImGuiPass;
        renderPassBI.framebuffer = m_ImGuiFrameBuffer->GetFrameBuffer(m_ImageIndex);
        renderPassBI.renderArea.offset = {0, 0};
        renderPassBI.renderArea.extent = m_VulkanSwapchain->GetExtent();
        renderPassBI.clearValueCount = 1;
        renderPassBI.pClearValues = &clearColor;

        vkCmdBeginRenderPass(ImGuiCommandBuffer, &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);
        {
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), ImGuiCommandBuffer);
        }
        vkCmdEndRenderPass(ImGuiCommandBuffer);
        vkEndCommandBuffer(ImGuiCommandBuffer);
    }
}

void VulkanRenderer::EndFrame() {
    vkResetFences(m_VulkanContext->GetDevice(), 1, &m_InFlightFences[m_CurrentFrame]);

    const std::vector<VkSemaphore> waitSemaphores{m_AvailableSemaphores[m_CurrentFrame]};
    const std::vector<VkSemaphore> signalSemaphores{m_FinishedSemaphores[m_CurrentFrame]};
    const std::vector<VkPipelineStageFlags> waitStages{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    std::array<VkCommandBuffer, 2> submitCommandBuffers = {
        m_VulkanFrameBuffer->GetCommandBuffer(m_CurrentFrame),
        m_ImGuiFrameBuffer->GetCommandBuffer(m_CurrentFrame)};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = waitStages.data();
    submitInfo.commandBufferCount = static_cast<uint32_t>(submitCommandBuffers.size());
    submitInfo.pCommandBuffers = submitCommandBuffers.data();
    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
    submitInfo.pSignalSemaphores = signalSemaphores.data();

    VkQueue graphicsQueue = m_VulkanContext->GetQueue(QueueIndex::eGraphics);
    CHK_RESULT((vkQueueSubmit(graphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrame]) == VK_SUCCESS),
        "Failed to submit draw command buffer!");

    const std::vector<VkSwapchainKHR> swapOld{m_VulkanSwapchain->GetSwapchain()};
    const std::vector<VkSemaphore> signalOld{m_FinishedSemaphores[m_CurrentFrame]};

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = static_cast<uint32_t>(swapOld.size());
    presentInfo.pSwapchains = swapOld.data();
    presentInfo.pImageIndices = &m_ImageIndex;
    presentInfo.waitSemaphoreCount = static_cast<uint32_t>(signalOld.size());
    presentInfo.pWaitSemaphores = signalOld.data();

    VkQueue presentQueue = m_VulkanContext->GetQueue(QueueIndex::ePresent);

    VkResult presentResult = vkQueuePresentKHR(presentQueue, &presentInfo);
    if (presentResult ==  VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || m_IsResized) {
        Resize();
    } else if (presentResult != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::Destroy() {
    VkDevice device = m_VulkanContext->GetDevice();

    vkDeviceWaitIdle(device);


    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    DestroyImgui(m_VulkanContext->GetDevice(), m_ImGuiPool, m_ImGuiPass);

    for (auto&& image : m_Images) {
        image.CleanupAll();
    }

    for (auto&& model : m_Models) {
        model.CleanupAll();
    }

    for (auto&& buffer : m_Buffers) {
        buffer.CleanupAll();
    }

    for (size_t i{0}, n = m_AvailableSemaphores.size(); i < n; ++i) {
        vkDestroySemaphore(device, m_AvailableSemaphores[i], nullptr);
        vkDestroySemaphore(device, m_FinishedSemaphores[i], nullptr);
        vkDestroyFence(device, m_InFlightFences[i], nullptr);
    }

    m_VulkanFrameBuffer->CleanupAll();
    m_VulkanPipeline->CleanupAll();
    m_VulkanSwapchain->CleanupAll();
    m_VulkanContext->CleanupAll();

    glfwDestroyWindow(m_Window);
    glfwTerminate();

    delete m_VulkanFrameBuffer;
    delete m_VulkanPipeline;
    delete m_VulkanSwapchain;
    delete m_VulkanContext;
}

void VulkanRenderer::SetIsResized(bool isResized_) {
    m_IsResized = isResized_;
}

void VulkanRenderer::RecreateSwapchain() {
    // TODO: doesn't work on UNIX
    int width = 0, height = 0;
    glfwGetFramebufferSize(m_Window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(m_Window, &width, &height);
        printf("\n %i%i", width,height);
        glfwWaitEvents();
    }

    VkDevice device = m_VulkanContext->GetDevice();
    vkDeviceWaitIdle(device);

    m_VulkanSwapchain->CleanupSwapchain();
    m_VulkanSwapchain->CleanupImageViews();
    // m_VulkanSwapchain->CleanupImages();
    m_VulkanFrameBuffer->CleanupColorResources();
    m_VulkanFrameBuffer->CleanupDepthResources();
    m_VulkanFrameBuffer->CleanupFrameBuffers();
    m_ImGuiFrameBuffer->CleanupFrameBuffers();

    CHK_RESULT(m_VulkanSwapchain->CreateSwapchain(m_Window),
        "Swapchain was not created!");

    CHK_RESULT(m_VulkanSwapchain->CreateImageViews(), 
        "Image views were not created!");

    CHK_RESULT(m_VulkanFrameBuffer->CreateDepthResources(), 
        "Depth Resources were not created!");

    CHK_RESULT(m_VulkanFrameBuffer->CreateColorResources(), 
        "Color Resources were not created!");

    CHK_RESULT(m_VulkanFrameBuffer->CreateFrameBuffers(m_VulkanPipeline->GetRenderPass()),
        "Frame buffers were not created!");

    CHK_RESULT(m_ImGuiFrameBuffer->CreateFrameBuffers(m_ImGuiPass, true),
        "Frame buffers were not created!");
}

void check_vk_result(VkResult err)
{
	if (err == 0)
		return;
	fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
	if (err < 0)
		abort();
}

void CreateImguiDescriptorPool(const VkDevice device_, VkDescriptorPool& pool_) {
    VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
	pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;
	vkCreateDescriptorPool(device_, &pool_info, nullptr, &pool_);
}

void createImGuiRenderPass(const VkFormat format_, const VkFormat depth_, const VkDevice device_, VkRenderPass& pass_) {
    std::array<VkAttachmentDescription, 1> attachmets({});
    attachmets[0].format = VK_FORMAT_B8G8R8A8_UNORM;
    attachmets[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachmets[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachmets[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmets[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmets[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmets[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachmets[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.attachmentCount = static_cast<uint32_t>(attachmets.size());
	info.pAttachments = attachmets.data();
	info.subpassCount = 1;
	info.pSubpasses = &subpass;
	info.dependencyCount = 1;
	info.pDependencies = &dependency;

	if (vkCreateRenderPass(device_, &info, nullptr, &pass_) != VK_SUCCESS)
		throw std::runtime_error("failed to create render pass!");
}

bool VulkanRenderer::InitImGui() {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

    ImGui_ImplGlfw_InitForVulkan(m_Window, true);
    ImGui_ImplVulkan_InitInfo info;
    info.Instance = m_VulkanContext->GetInstance();
    info.PhysicalDevice = m_VulkanContext->GetPhysicalDevice();
    info.Device = m_VulkanContext->GetDevice();
    info.QueueFamily = m_VulkanContext->GetQueueIndexes().GetQueueIndex(QueueIndex::eGraphics);
    info.Queue = m_VulkanContext->GetQueue(QueueIndex::eGraphics);
    info.PipelineCache = 0;
    CreateImguiDescriptorPool(m_VulkanContext->GetDevice(), m_ImGuiPool);
    info.DescriptorPool = m_ImGuiPool;
    info.Subpass = 0;
    info.MinImageCount = MAX_FRAMES_IN_FLIGHT;
    info.ImageCount = m_VulkanSwapchain->GetImageCount();
    info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    info.UseDynamicRendering = false;
    info.Allocator = 0;
    info.CheckVkResultFn = check_vk_result;
    createImGuiRenderPass(VK_FORMAT_B8G8R8A8_UNORM, m_VulkanPipeline->FindDepthFormat(), m_VulkanContext->GetDevice(), m_ImGuiPass);
    ImGui_ImplVulkan_Init(&info, m_ImGuiPass);

    m_ImGuiFrameBuffer = new VulkanFrameBuffer(m_VulkanContext, m_VulkanSwapchain, m_VulkanPipeline);
    m_ImGuiFrameBuffer->CreateCommandPool();
    m_ImGuiFrameBuffer->CreateFrameBuffers(m_ImGuiPass, true);
    m_ImGuiFrameBuffer->CreateCommandBuffer(MAX_FRAMES_IN_FLIGHT);

    ImGui_ImplVulkan_CreateFontsTexture();

    // createImGuiTextureSampler(m_VulkanContext->GetDevice(), m_ImGuiSampler);
    // auto&& images = m_VulkanSwapchain->GetImages();
    // m_Dest.resize(images.size());
    // for (size_t i{0}, n{m_Dest.size()}; i < n; ++i) {
    //     m_Dest[i] = ImGui_ImplVulkan_AddTexture(m_ImGuiSampler, images[i].GetImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    // }

    // TODO: Need create frame buffer for Imgui
    // then in recordBuffer function create commandBuffer
    // use this buffer for ImGui_ImpleVulkan_RenderDrawData()
    // then submit both command buffer to queue
    return true;
}

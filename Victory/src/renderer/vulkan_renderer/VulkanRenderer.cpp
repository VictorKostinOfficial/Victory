#include <vulkan/vulkan.h>

#include "VulkanRenderer.h"

#include <GLFW/glfw3.h>
#include <stdexcept>
#include <stdio.h>
#include <array>
#include <iostream>

#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanBuffer.h"
#include "VulkanFrameBuffer.h"
#include "VulkanUtils.h"
#include "VulkanImage.h"
#include "VulkanModel.h"

#include "../../Utils.h"
#include "VulkanVertexData.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

void check_vk_result(VkResult err)
{
	if (err == 0)
		return;
	fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
	if (err < 0)
		abort();
}

class ImGuiPipeline {
public:

    ImGuiPipeline(VulkanContext* context_, VulkanSwapchain* swapchain_, GLFWwindow* window_) 
        : m_Context{ context_ }
        , m_Swapchain{ swapchain_ }
        , m_Window{ window_ } {
    }

    ~ImGuiPipeline() {
        CleanupDescriptorSets();
        vkDestroySampler(m_Context->GetDevice(), m_ViewportSampler, nullptr);
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        m_FrameBuffer->CleanupAll();
        delete m_FrameBuffer;

        vkDestroyPipeline(m_Context->GetDevice(), m_Pipeline, nullptr);
        vkDestroyShaderModule(m_Context->GetDevice(), FS, nullptr);
        vkDestroyShaderModule(m_Context->GetDevice(), VS, nullptr);
        vkDestroyPipelineLayout(m_Context->GetDevice(), m_PipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(m_Context->GetDevice(), m_DescriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(m_Context->GetDevice(), m_DescriptorPool, nullptr);
        vkDestroyRenderPass(m_Context->GetDevice(), m_RenderPass, nullptr);
    }

    void Init() {
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
    };

    void InitResources(const uint32_t minImageCount_) {
        CreateRenderPass();
        CreateDescriptorPool();
        CreateDescriptorSetLayout();
        CreatePipelineLayout();
        CreatePipeline();
        InitImGui(minImageCount_);
        CreateFrameBuffer(minImageCount_);
    };

    void NewFrame(const uint32_t currentFrame_) {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGuiIO &io = ImGui::GetIO();
        ImGui::NewFrame();
        {
            ImGui::ShowDemoWindow();
            ImGui::ShowMetricsWindow();
        
            // ImGui::Begin("Viewport");
            // ImGuiIO& io = ImGui::GetIO();
		    // ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
            // ImGui::Image(m_DescriptorSets[currentFrame_], ImVec2{viewportPanelSize.x,viewportPanelSize.y});
		    // ImGui::End();
        }
        ImGui::Render();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    };

    void DrawFrame(const uint32_t currentFrmae_, const uint32_t indexImage_) {
        VkCommandBuffer commandBuffer = m_FrameBuffer->GetCommandBuffer(currentFrmae_);

        VkCommandBufferBeginInfo commandBufferBI{};
        commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &commandBufferBI);
        {
            VkClearValue clearColor = {0.f, 0.f, 0.f, 0.f};

            VkRenderPassBeginInfo renderPassBI{};
            renderPassBI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBI.renderPass = m_RenderPass;
            renderPassBI.framebuffer = m_FrameBuffer->GetFrameBuffer(indexImage_);
            renderPassBI.renderArea.offset = {0, 0};
            renderPassBI.renderArea.extent = m_Swapchain->GetExtent();
            renderPassBI.clearValueCount = 1;
            renderPassBI.pClearValues = &clearColor;

            vkCmdBeginRenderPass(commandBuffer, &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);
            {
                ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
            }
            vkCmdEndRenderPass(commandBuffer);
        }
        vkEndCommandBuffer(commandBuffer);
    };
    
    void InitDescriptorSets(const std::vector<VulkanImage>& images_) {
        CreateSampler();

        const size_t imageCount = images_.size();
        m_DescriptorSets.resize(imageCount);
        for (size_t i{ 0 }; i < imageCount; ++i) {
            m_DescriptorSets[i] = ImGui_ImplVulkan_AddTexture(m_ViewportSampler, 
                images_[i].GetImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    }

    void RecreateFrameBuffer() {
        m_FrameBuffer->CleanupFrameBuffers();
        m_FrameBuffer->CreateFrameBuffers(m_RenderPass, m_Swapchain->GetExtent(), m_Swapchain->GetImages());
    }

    void RecreateDescriptorSets(const std::vector<VulkanImage>& images_) {
        CleanupDescriptorSets();

        const size_t imageCount = images_.size();
        m_DescriptorSets.resize(imageCount);
        for (size_t i{ 0 }; i < imageCount; ++i) {
            m_DescriptorSets[i] = ImGui_ImplVulkan_AddTexture(m_ViewportSampler, 
                images_[i].GetImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    }

    inline VulkanFrameBuffer* GetVulkanFrameBuffer() {
        return m_FrameBuffer;
    }    

    inline VkCommandBuffer GetCommandBuffer(const uint32_t currentFrame_) {
        return m_FrameBuffer->GetCommandBuffer(currentFrame_);
    }

    inline VkDescriptorSetLayout GetDescriptorSetLayout() {
        return m_DescriptorSetLayout;
    }

private:

    void InitImGui(const uint32_t minImageCount_) {

        ImGui_ImplGlfw_InitForVulkan(m_Window, true);
        ImGui_ImplVulkan_InitInfo info;
        info.Instance = m_Context->GetInstance();
        info.PhysicalDevice = m_Context->GetPhysicalDevice();
        info.Device = m_Context->GetDevice();
        info.QueueFamily = m_Context->GetQueueIndexes().GetQueueIndex(QueueIndex::eGraphics);
        m_Context->GetQueue(info.Queue, QueueIndex::eGraphics);
        info.PipelineCache = 0;
        info.DescriptorPool = m_DescriptorPool;
        info.Subpass = 0;
        info.MinImageCount = minImageCount_;
        info.ImageCount = m_Swapchain->GetImageCount();
        info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        info.UseDynamicRendering = false;
        info.Allocator = 0;
        info.CheckVkResultFn = check_vk_result;
        ImGui_ImplVulkan_Init(&info, m_RenderPass);
        ImGui_ImplVulkan_CreateFontsTexture();
    }

    void CreateRenderPass() {
        std::array<VkAttachmentDescription, 1> attachmets({});
        attachmets[0].format = m_Swapchain->GetSurfaceFormat().format;
        attachmets[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachmets[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmets[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmets[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmets[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmets[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
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

	    if (vkCreateRenderPass(m_Context->GetDevice(), &info, nullptr, &m_RenderPass) != VK_SUCCESS){
		    throw std::runtime_error("Failed to create render pass!");
        }
    }

    void CreateDescriptorPool() {
        // TODO: Do smth convenient 
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
	    if (vkCreateDescriptorPool(m_Context->GetDevice(), &pool_info, nullptr, &m_DescriptorPool) != VK_SUCCESS) {
		    throw std::runtime_error("Failed to create descriptor pool!");
        }
    }

    void CreateDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 0;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        const std::array<VkDescriptorSetLayoutBinding, 1>  bindings{ samplerLayoutBinding };

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
        descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCI.bindingCount = static_cast<uint32_t>(bindings.size());
        descriptorSetLayoutCI.pBindings = bindings.data();

        vkCreateDescriptorSetLayout(m_Context->GetDevice(), &descriptorSetLayoutCI, 
            nullptr, &m_DescriptorSetLayout);
    }

    void CreatePipelineLayout() {
        VkPipelineLayoutCreateInfo pipelineLayoutCI{};
        pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCI.setLayoutCount = 1;
        pipelineLayoutCI.pSetLayouts = &m_DescriptorSetLayout;
        pipelineLayoutCI.pushConstantRangeCount = 0;
        pipelineLayoutCI.pPushConstantRanges = nullptr;

        vkCreatePipelineLayout(m_Context->GetDevice(), &pipelineLayoutCI, 
            nullptr, &m_PipelineLayout);
    }

    std::vector<VkVertexInputBindingDescription> GetBindingDescription() {
        std::vector<VkVertexInputBindingDescription> bindingDescription{ 1 };
        bindingDescription[0].binding = 0;
        bindingDescription[0].stride = sizeof(ImDrawVert);
        bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    };

    std::vector<VkVertexInputAttributeDescription> GetAttriburteDesciption() {
        std::vector<VkVertexInputAttributeDescription> attribDescription{ 3 };

        attribDescription[0].binding = 0;
        attribDescription[0].location = 0;
        attribDescription[0].format = VK_FORMAT_R32G32_SFLOAT;
        attribDescription[0].offset = 0;

        attribDescription[1].binding = 0;
        attribDescription[1].location = 1;
        attribDescription[1].format = VK_FORMAT_R32G32_SFLOAT;
        attribDescription[1].offset = sizeof(float) * 2;

        attribDescription[2].binding = 0;
        attribDescription[2].location = 2;
        attribDescription[2].format = VK_FORMAT_R8G8B8A8_UNORM;
        attribDescription[2].offset = sizeof(float) * 4;

        return attribDescription;
    }
    
    void CreatePipeline() {
        const std::vector<char> vsBuffer = Utils::ReadFile("ui.vert.spv");
        const std::vector<char> fsBuffer = Utils::ReadFile("ui.frag.spv");
        if (LoadShader(vsBuffer, &VS) != true) {
            printf("\nVertex shader was not loaded!");
            // TODO: Errors...
            return;
        }
        if (LoadShader(fsBuffer, &FS) != true) {
            printf("\nFragment shader was not loaded!");
            return;
        }

        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStageCIs({});
        shaderStageCIs[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageCIs[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStageCIs[0].module = VS;
        shaderStageCIs[0].pName = "main";
        shaderStageCIs[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageCIs[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStageCIs[1].module = FS;
        shaderStageCIs[1].pName = "main";

        // TODO: CreateBindingDescription
        auto&& bindingDescription = GetBindingDescription();
        auto&& attributegDescriptions = GetAttriburteDesciption();

        VkPipelineVertexInputStateCreateInfo vertexInputStateCI{};
        vertexInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputStateCI.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescription.size());
        vertexInputStateCI.pVertexBindingDescriptions = bindingDescription.data();
        vertexInputStateCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributegDescriptions.size());
        vertexInputStateCI.pVertexAttributeDescriptions = attributegDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{};
        inputAssemblyStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssemblyStateCI.primitiveRestartEnable = VK_FALSE;

        VkPipelineTessellationStateCreateInfo tessellationStateCI{};
        tessellationStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;

        VkPipelineViewportStateCreateInfo viewportStateCI{};
        viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCI.viewportCount = 1;
        viewportStateCI.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizationStateCI{};
        rasterizationStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationStateCI.depthClampEnable = VK_FALSE;
        rasterizationStateCI.rasterizerDiscardEnable = VK_FALSE;
        rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationStateCI.lineWidth = 1.f;
        rasterizationStateCI.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizationStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizationStateCI.depthBiasEnable = VK_FALSE;
        rasterizationStateCI.depthBiasConstantFactor = 0.f;
        rasterizationStateCI.depthBiasClamp = 0.f;
        rasterizationStateCI.depthBiasSlopeFactor = 0.f;

        VkPipelineMultisampleStateCreateInfo multisampleStateCI{};
        multisampleStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleStateCI.sampleShadingEnable = VK_FALSE;
        multisampleStateCI.minSampleShading = .2f;
        multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};
        depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilStateCI.depthTestEnable = false;
        depthStencilStateCI.depthWriteEnable = VK_TRUE;
        depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencilStateCI.depthBoundsTestEnable = VK_FALSE;
        depthStencilStateCI.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
        colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
                                                | VK_COLOR_COMPONENT_G_BIT
                                                | VK_COLOR_COMPONENT_B_BIT
                                                | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachmentState.blendEnable = VK_FALSE;
        colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlendStateCI{};
        colorBlendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendStateCI.logicOpEnable = VK_FALSE;
        colorBlendStateCI.logicOp = VK_LOGIC_OP_COPY;
        colorBlendStateCI.attachmentCount = 1;
        colorBlendStateCI.pAttachments = &colorBlendAttachmentState;
        colorBlendStateCI.blendConstants[0] = 0.f;
        colorBlendStateCI.blendConstants[1] = 0.f;
        colorBlendStateCI.blendConstants[2] = 0.f;
        colorBlendStateCI.blendConstants[3] = 0.f;

        const std::array<VkDynamicState, 2> dynamicStates{
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicStateCI{};
        dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCI.dynamicStateCount = 2;
        dynamicStateCI.pDynamicStates = dynamicStates.data();

        VkGraphicsPipelineCreateInfo pipelineCI{};
        pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCI.stageCount = static_cast<uint32_t>(shaderStageCIs.size());
        pipelineCI.pStages = shaderStageCIs.data();
        pipelineCI.pVertexInputState = &vertexInputStateCI;
        pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
        pipelineCI.pTessellationState = &tessellationStateCI;
        pipelineCI.pViewportState = &viewportStateCI;
        pipelineCI.pRasterizationState = &rasterizationStateCI;
        pipelineCI.pMultisampleState = &multisampleStateCI;
        pipelineCI.pDepthStencilState = &depthStencilStateCI;
        pipelineCI.pColorBlendState = &colorBlendStateCI;
        pipelineCI.pDynamicState = &dynamicStateCI;
        pipelineCI.layout = m_PipelineLayout;
        pipelineCI.renderPass = m_RenderPass;
        pipelineCI.subpass = 0;
        pipelineCI.basePipelineHandle = VK_NULL_HANDLE;
        pipelineCI.basePipelineIndex = -1;

        vkCreateGraphicsPipelines(m_Context->GetDevice(), nullptr, 1, 
            &pipelineCI, nullptr, &m_Pipeline);
    }

    void CreateFrameBuffer(const uint32_t minImageCount_) {
        m_FrameBuffer = new VulkanFrameBuffer(m_Context);
        m_FrameBuffer->CreateCommandPool(QueueIndex::eGraphics);
        m_FrameBuffer->CreateFrameBuffers(m_RenderPass, m_Swapchain->GetExtent(), 
                m_Swapchain->GetImages(), true);
        m_FrameBuffer->CreateCommandBuffer(minImageCount_);
    }

    void CreateSampler() {
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

	    vkCreateSampler(m_Context->GetDevice(), &samplerInfo, nullptr, &m_ViewportSampler);
    }

    bool LoadShader(const std::vector<char>&buffer_, VkShaderModule* shaderModule_) {
        if (!shaderModule_) {
            return false;
        }

        VkShaderModuleCreateInfo shaderModuleCI{};
        shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCI.codeSize = static_cast<uint32_t>(buffer_.size());
        shaderModuleCI.pCode = reinterpret_cast<const uint32_t*>(buffer_.data());

        return vkCreateShaderModule(m_Context->GetDevice(), &shaderModuleCI, 
            nullptr, shaderModule_) == VK_SUCCESS;
    }

    void CleanupDescriptorSets() {

        for (auto&& desc : m_DescriptorSets) {
            ImGui_ImplVulkan_RemoveTexture(desc);
        }
    }

private:

    GLFWwindow* m_Window{ nullptr };
    VulkanContext* m_Context{ nullptr };
    VulkanSwapchain* m_Swapchain{ nullptr };
    VulkanFrameBuffer* m_FrameBuffer{ VK_NULL_HANDLE };

    VkPipelineCache m_PipelineCash{ VK_NULL_HANDLE };
    VkDescriptorSetLayout m_DescriptorSetLayout{ VK_NULL_HANDLE };
    VkPipelineLayout m_PipelineLayout{ VK_NULL_HANDLE };
    VkRenderPass m_RenderPass{ VK_NULL_HANDLE };
    VkPipeline m_Pipeline{ VK_NULL_HANDLE };

    VkDescriptorPool m_DescriptorPool{ VK_NULL_HANDLE };

    VkSampler m_ViewportSampler{ VK_NULL_HANDLE };
    std::vector<VkDescriptorSet> m_DescriptorSets;

    VkShaderModule VS{VK_NULL_HANDLE};
    VkShaderModule FS{VK_NULL_HANDLE};
};

class ViewportPipeline {
public:

    ViewportPipeline(VulkanContext* context_, VulkanSwapchain* swapchain_) 
        : m_Context{ context_ }
        , m_Swapchain{ swapchain_ } {
    }

    ~ViewportPipeline() {
        for (auto&& image : m_Images) {
            image.CleanupImageView();
            image.FreeImageMemory();
            image.CleanupImage();
        }

        m_FrameBuffer->CleanupAll();
        delete m_FrameBuffer;

        vkDestroyShaderModule(m_Context->GetDevice(), FS, nullptr);
        vkDestroyShaderModule(m_Context->GetDevice(), VS, nullptr);

        vkDestroyPipeline(m_Context->GetDevice(), m_Pipeline, nullptr);
        vkDestroyRenderPass(m_Context->GetDevice(), m_RenderPass, nullptr);

        vkDestroyPipelineLayout(m_Context->GetDevice(), m_PipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(m_Context->GetDevice(), m_DescriptorSetLayout, nullptr);
    }

    void InitResources(const uint32_t minImageCount_) {
        CreateDescriptorSetLayout();
        CreatePipelineLayout();

        CreateRenderPass();
        CreatePipeline();

        CreateFrameBuffers(minImageCount_);
    }

    void DrawFrame(const uint32_t curentFrame_, const uint32_t imageIndex_ 
            , std::vector<VulkanModel>& modes_, std::vector<VulkanBuffer>& buffers_) {
        VkCommandBuffer commandBuffer = m_FrameBuffer->GetCommandBuffer(curentFrame_);

        VkCommandBufferBeginInfo commandBufferBI{};
        commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &commandBufferBI);

        VkRect2D rect{};
        rect.offset = {0, 0};
        rect.extent = m_Swapchain->GetExtent();

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.f, 0.f, 0.f, 1.f}};
        clearValues[1].depthStencil = {1.f, 0};

        VkRenderPassBeginInfo renderPassBI{};
        renderPassBI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBI.renderPass = m_RenderPass;
        renderPassBI.framebuffer = m_FrameBuffer->GetFrameBuffer(imageIndex_);
        renderPassBI.renderArea = rect;
        renderPassBI.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassBI.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);
        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

            VkViewport viewport{};
            viewport.x = 0.f;
            viewport.y = 0.f;
            viewport.width = static_cast<float>(rect.extent.width);
            viewport.height = static_cast<float>(rect.extent.height);
            viewport.minDepth = 0.f;
            viewport.maxDepth = 1.f;

            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
            vkCmdSetScissor(commandBuffer, 0, 1, &rect);

            for (size_t i{0}, n = modes_.size(); i < n; ++i) {
                std::vector<VkBuffer> vertexBuffers{modes_[i].GetVertexBuffer()};
                VkBuffer indexBuffer{modes_[i].GetIndexBuffer()};
                std::vector<VkDeviceSize> offsets{0};

                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets.data());
                vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 
                    0, 1, &buffers_[i].GetDescriptorSet(curentFrame_), 0, nullptr);

                vkCmdDrawIndexed(commandBuffer, modes_[i].GetIndexCount(), 1, 0, 0, 0);
            }
        }
        vkCmdEndRenderPass(commandBuffer);
        vkEndCommandBuffer(commandBuffer);
    }

    void RecreateFrameBuffer() {
        m_FrameBuffer->CleanupFrameBuffers();
        
        for (auto&& image : m_Images)
        {
            image.CleanupImageView();
            image.FreeImageMemory();
            image.CleanupImage();
        }

        m_Images.resize(m_Swapchain->GetImageCount(), VulkanImage(m_Context, m_FrameBuffer));
        for (auto&& image : m_Images) {
            CreateImageSettings viewportImageSettings{};
            viewportImageSettings.Width = m_Swapchain->GetExtent().width;
            viewportImageSettings.Height =  m_Swapchain->GetExtent().height;
            viewportImageSettings.Format = m_Swapchain->GetSurfaceFormat().format;
            viewportImageSettings.Tiling = VK_IMAGE_TILING_LINEAR;
            viewportImageSettings.Usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            viewportImageSettings.Properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            viewportImageSettings.SampleCount = VK_SAMPLE_COUNT_1_BIT;
            image.CreateImage(viewportImageSettings);
            image.CreateImageView(viewportImageSettings.Format, VK_IMAGE_ASPECT_COLOR_BIT);
        }

        m_FrameBuffer->CreateFrameBuffers(m_RenderPass, m_Swapchain->GetExtent(), m_Images);
    }

    inline std::vector<VulkanImage>& GetImages() {
        return m_Images;
    }

    inline VkCommandBuffer GetCommandBuffer(const uint32_t currentFrame_) {
        return m_FrameBuffer->GetCommandBuffer(currentFrame_);
    }

    inline VulkanFrameBuffer* GetVulkanFrameBuffer() {
        return m_FrameBuffer;
    }

    inline VkDescriptorSetLayout GetDescriptorSetLayout() {
        return m_DescriptorSetLayout;
    }

private:

    void CreateRenderPass() {
        // VkAttachmentDescription colorAttachment{};
        // colorAttachment.format = format_;
        // colorAttachment.samples = m_Context->GetSampleCount();
        // colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        // colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        // colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        // colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        // colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        // colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // VkAttachmentDescription depthAttachment{};
        // depthAttachment.format = FindDepthFormat();
        // depthAttachment.samples = m_Context->GetSampleCount();
        // depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        // depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        // depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        // depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        // depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        // depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = m_Swapchain->GetSurfaceFormat().format;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        // VkAttachmentReference colorAttachmentRef{};
        // colorAttachmentRef.attachment = 0;
        // colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // VkAttachmentReference depthAttachmentRef{};
        // depthAttachmentRef.attachment = 1;
        // depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        // subpass.pDepthStencilAttachment = &depthAttachmentRef;
        // subpass.pResolveAttachments = &colorAttachmentResolveRef;
        // The index of the attachment in this array is directly referenced from the 
        // fragment shader with the layout(location = 0) out vec4 outColor directive!

        std::array<VkSubpassDependency, 1> dependencies;
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependencies[0].srcAccessMask = 0;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = 0;

        // std::array<VkAttachmentDescription, 3> attachmets{colorAttachment, depthAttachment, colorAttachmentResolve};
        std::array<VkAttachmentDescription, 1> attachmets{ colorAttachment };

        VkRenderPassCreateInfo renderPassCI{};
        renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCI.attachmentCount = static_cast<uint32_t>(attachmets.size());
        renderPassCI.pAttachments = attachmets.data();
        renderPassCI.subpassCount = 1;
        renderPassCI.pSubpasses = &subpass;
        renderPassCI.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassCI.pDependencies = dependencies.data();

        vkCreateRenderPass(m_Context->GetDevice(), 
            &renderPassCI, nullptr, &m_RenderPass);
    }

    void CreateDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2>  bindings{uboLayoutBinding, samplerLayoutBinding};

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
        descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCI.bindingCount = static_cast<uint32_t>(bindings.size());
        descriptorSetLayoutCI.pBindings = bindings.data();

        vkCreateDescriptorSetLayout(m_Context->GetDevice(), 
            &descriptorSetLayoutCI, nullptr, &m_DescriptorSetLayout);
    }

    void CreatePipelineLayout() {
        VkPipelineLayoutCreateInfo pipelineLayoutCI{};
        pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCI.setLayoutCount = 1;
        pipelineLayoutCI.pSetLayouts = &m_DescriptorSetLayout;
        pipelineLayoutCI.pushConstantRangeCount = 0;
        pipelineLayoutCI.pPushConstantRanges = nullptr;

        vkCreatePipelineLayout(m_Context->GetDevice(), &pipelineLayoutCI, 
            nullptr, &m_PipelineLayout);
    }

    void CreatePipeline() {
        std::vector<char> vsBuffer = Utils::ReadFile("graphics.vert.spv");
        std::vector<char> fsBuffer = Utils::ReadFile("graphics.frag.spv");
        if (LoadShader(vsBuffer, &VS) != true) {
            printf("\nVertex shader was not loaded!");
            return;
        }
        if (LoadShader(fsBuffer, &FS) != true) {
            printf("\nFragment shader was not loaded!");
            return;
        }

        VkPipelineShaderStageCreateInfo vertexShaderStageCI{};
        vertexShaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertexShaderStageCI.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertexShaderStageCI.module = VS;
        vertexShaderStageCI.pName = "main";
        VkPipelineShaderStageCreateInfo fragmentShaderStageCI{};
        fragmentShaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragmentShaderStageCI.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragmentShaderStageCI.module = FS;
        fragmentShaderStageCI.pName = "main";

        const std::vector<VkPipelineShaderStageCreateInfo> shaderStageCIs{
            vertexShaderStageCI,
            fragmentShaderStageCI
        };

        auto&& bindingDescription = VulkanRendererSpace::GetBindingDescription();
        auto&& attributegDescriptions = VulkanRendererSpace::GetAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vertexInputStateCI{};
        vertexInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputStateCI.vertexBindingDescriptionCount = 1;
        vertexInputStateCI.pVertexBindingDescriptions = &bindingDescription;
        vertexInputStateCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributegDescriptions.size());
        vertexInputStateCI.pVertexAttributeDescriptions = attributegDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{};
        inputAssemblyStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssemblyStateCI.primitiveRestartEnable = VK_FALSE;

        VkPipelineTessellationStateCreateInfo tessellationStateCI{};
        tessellationStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;

        VkPipelineViewportStateCreateInfo viewportStateCI{};
        viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCI.viewportCount = 1;
        viewportStateCI.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizationStateCI{};
        rasterizationStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationStateCI.depthClampEnable = VK_FALSE;
        rasterizationStateCI.rasterizerDiscardEnable = VK_FALSE;
        rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationStateCI.lineWidth = 1.f;
        rasterizationStateCI.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizationStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizationStateCI.depthBiasEnable = VK_FALSE;
        rasterizationStateCI.depthBiasConstantFactor = 0.f;
        rasterizationStateCI.depthBiasClamp = 0.f;
        rasterizationStateCI.depthBiasSlopeFactor = 0.f;

        VkPipelineMultisampleStateCreateInfo multisampleStateCI{};
        multisampleStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleStateCI.sampleShadingEnable = VK_FALSE;
        multisampleStateCI.minSampleShading = .2f;
        // multisampleStateCI.rasterizationSamples = m_Context->GetSampleCount();
        multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};
        depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilStateCI.depthTestEnable = VK_TRUE;
        depthStencilStateCI.depthWriteEnable = VK_TRUE;
        depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencilStateCI.depthBoundsTestEnable = VK_FALSE;
        depthStencilStateCI.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
        colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
                                                | VK_COLOR_COMPONENT_G_BIT
                                                | VK_COLOR_COMPONENT_B_BIT
                                                | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachmentState.blendEnable = VK_FALSE;
        colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlendStateCI{};
        colorBlendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendStateCI.logicOpEnable = VK_FALSE;
        colorBlendStateCI.logicOp = VK_LOGIC_OP_COPY;
        colorBlendStateCI.attachmentCount = 1;
        colorBlendStateCI.pAttachments = &colorBlendAttachmentState;
        colorBlendStateCI.blendConstants[0] = 0.f;
        colorBlendStateCI.blendConstants[1] = 0.f;
        colorBlendStateCI.blendConstants[2] = 0.f;
        colorBlendStateCI.blendConstants[3] = 0.f;

        const std::vector<VkDynamicState> dynamicStates{
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicStateCI{};
        dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCI.dynamicStateCount = 2;
        dynamicStateCI.pDynamicStates = dynamicStates.data();

        VkGraphicsPipelineCreateInfo pipelineCI{};
        pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCI.stageCount = 2;
        pipelineCI.pStages = shaderStageCIs.data();
        pipelineCI.pVertexInputState = &vertexInputStateCI;
        pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
        pipelineCI.pTessellationState = &tessellationStateCI;
        pipelineCI.pViewportState = &viewportStateCI;
        pipelineCI.pRasterizationState = &rasterizationStateCI;
        pipelineCI.pMultisampleState = &multisampleStateCI;
        pipelineCI.pDepthStencilState = &depthStencilStateCI;
        pipelineCI.pColorBlendState = &colorBlendStateCI;
        pipelineCI.pDynamicState = &dynamicStateCI;
        pipelineCI.layout = m_PipelineLayout;
        pipelineCI.renderPass = m_RenderPass;
        pipelineCI.subpass = 0; // Index of render pass subpass
        pipelineCI.basePipelineHandle = VK_NULL_HANDLE;
        pipelineCI.basePipelineIndex = -1;

        vkCreateGraphicsPipelines(m_Context->GetDevice(), 
            nullptr, 1, &pipelineCI, nullptr, &m_Pipeline);

    }

    void CreateFrameBuffers(const uint32_t minImageCount_) {
        m_FrameBuffer = new VulkanFrameBuffer(m_Context);
        m_FrameBuffer->CreateCommandPool(QueueIndex::eGraphics);
      
        // m_ViewportFrameBuffer->CreateDepthResources(m_VulkanPipeline->FindDepthFormat()
        //         , m_VulkanSwapchain->GetExtent());
        // m_ViewportFrameBuffer->CreateColorResources(m_VulkanSwapchain->GetSurfaceFormat().format
        //         , m_VulkanSwapchain->GetExtent());

        m_Images.resize(m_Swapchain->GetImageCount(), VulkanImage(m_Context, m_FrameBuffer));
        for (auto&& image : m_Images) {
            CreateImageSettings viewportImageSettings{};
            viewportImageSettings.Width = m_Swapchain->GetExtent().width;
            viewportImageSettings.Height =  m_Swapchain->GetExtent().height;
            viewportImageSettings.Format = m_Swapchain->GetSurfaceFormat().format;
            viewportImageSettings.Tiling = VK_IMAGE_TILING_LINEAR;
            viewportImageSettings.Usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            viewportImageSettings.Properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            viewportImageSettings.SampleCount = VK_SAMPLE_COUNT_1_BIT;
            image.CreateImage(viewportImageSettings);
            image.CreateImageView(viewportImageSettings.Format, VK_IMAGE_ASPECT_COLOR_BIT);
        }

        m_FrameBuffer->CreateFrameBuffers(m_RenderPass, m_Swapchain->GetExtent(), m_Images);
        m_FrameBuffer->CreateCommandBuffer(minImageCount_);
    }

    bool LoadShader(const std::vector<char>&buffer_, VkShaderModule* shaderModule_) {
        if (!shaderModule_) {
            return false;
        }

        VkShaderModuleCreateInfo shaderModuleCI{};
        shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCI.codeSize = static_cast<uint32_t>(buffer_.size());
        shaderModuleCI.pCode = reinterpret_cast<const uint32_t*>(buffer_.data());

        return vkCreateShaderModule(m_Context->GetDevice(), &shaderModuleCI, 
            nullptr, shaderModule_) == VK_SUCCESS;
    }

private:

    VulkanContext* m_Context{ nullptr };
    VulkanSwapchain* m_Swapchain{ nullptr };
    VulkanFrameBuffer* m_FrameBuffer{ nullptr };

    VkRenderPass m_RenderPass{ VK_NULL_HANDLE };
    VkPipeline m_Pipeline{ VK_NULL_HANDLE };

    VkDescriptorSetLayout m_DescriptorSetLayout{ VK_NULL_HANDLE };
    VkPipelineLayout m_PipelineLayout{ VK_NULL_HANDLE };

    std::vector<VulkanImage> m_Images;

    VkShaderModule VS{VK_NULL_HANDLE};
    VkShaderModule FS{VK_NULL_HANDLE};
};

static ImGuiPipeline* m_ImGuiPipeline{ nullptr };
static ViewportPipeline* m_ViewportPipeline{ nullptr };

#define CHK_RESULT(RESULT, MESSAGE) \
    if (RESULT == false) { \
        std::cout << MESSAGE << std::endl; \
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

    CHK_RESULT(m_VulkanSwapchain->CreateImages(nullptr),
        "Surface images were not created!");

    CHK_RESULT(m_VulkanSwapchain->CreateImageViews(VK_IMAGE_ASPECT_COLOR_BIT), 
        "Image views were not created!");

    m_ViewportPipeline = new ViewportPipeline(m_VulkanContext, m_VulkanSwapchain);
    m_ViewportPipeline->InitResources(MAX_FRAMES_IN_FLIGHT);

    m_ImGuiPipeline = new ImGuiPipeline(m_VulkanContext, m_VulkanSwapchain, m_Window);
    m_ImGuiPipeline->Init();
    m_ImGuiPipeline->InitResources(MAX_FRAMES_IN_FLIGHT);
    m_ImGuiPipeline->InitDescriptorSets(m_ViewportPipeline->GetImages());

    {
        m_Models.push_back(VulkanModel(m_VulkanContext, m_ViewportPipeline->GetVulkanFrameBuffer()));
        m_Models[0].LoadModel("viking_room.obj");
        m_Models[0].CreateVertexBuffer();
        m_Models[0].CreateIndexBuffer();

        m_Images.push_back(VulkanImage(m_VulkanContext, m_ViewportPipeline->GetVulkanFrameBuffer()));
        CreateImageSettings settings{};
        settings.Format = VK_FORMAT_R8G8B8A8_UNORM;
        settings.Tiling = VK_IMAGE_TILING_OPTIMAL;
        settings.Usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        settings.Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        settings.SampleCount = VK_SAMPLE_COUNT_1_BIT;
        m_Images[0].LoadTexture("viking_room.png", settings);
        m_Images[0].CreateImageView(settings.Format, VK_IMAGE_ASPECT_COLOR_BIT);
        m_Images[0].CreateSampler();

        m_Buffers.push_back(VulkanBuffer(m_VulkanContext, 
            m_ViewportPipeline->GetVulkanFrameBuffer(), m_VulkanSwapchain));
        for (size_t i{0}, n = m_Buffers.size(); i < n; ++i) {
            CHK_RESULT(m_Buffers[i].CreateUniformBuffers(MAX_FRAMES_IN_FLIGHT),
                "Uniform buffer was not created");

            CHK_RESULT(m_Buffers[i].CreateDescriptorPool(MAX_FRAMES_IN_FLIGHT),
                "Descriptor pool was not created");

            CHK_RESULT(m_Buffers[i].CreateDescriptorSets(MAX_FRAMES_IN_FLIGHT, m_Images[i].GetSampler(), 
                m_Images[i].GetImageView(), m_ViewportPipeline->GetDescriptorSetLayout()),
                "Descriptor sets was not created");
        }
    }

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
    // printf("\n-------------- BEGIN FRAME -------------");
    for (size_t i{0}, n = m_Buffers.size(); i < n; ++i) {
        m_Buffers[i].UpdateUniformBuffer(m_CurrentFrame);
    }
}

void VulkanRenderer::RecordCommandBuffer() {
    m_ImGuiPipeline->NewFrame(m_CurrentFrame);
    m_ImGuiPipeline->DrawFrame(m_CurrentFrame, m_ImageIndex);
    m_ViewportPipeline->DrawFrame(m_CurrentFrame, m_ImageIndex, m_Models, m_Buffers);
}

void VulkanRenderer::EndFrame() {
    vkResetFences(m_VulkanContext->GetDevice(), 1, &m_InFlightFences[m_CurrentFrame]);

    const std::vector<VkSemaphore> waitSemaphores{m_AvailableSemaphores[m_CurrentFrame]};
    const std::vector<VkSemaphore> signalSemaphores{m_FinishedSemaphores[m_CurrentFrame]};
    const std::vector<VkPipelineStageFlags> waitStages{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    std::array<VkCommandBuffer, 2> submitCommandBuffers = {
        m_ImGuiPipeline->GetCommandBuffer(m_CurrentFrame),
        m_ViewportPipeline->GetCommandBuffer(m_CurrentFrame)};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = waitStages.data();
    submitInfo.commandBufferCount = static_cast<uint32_t>(submitCommandBuffers.size());
    submitInfo.pCommandBuffers = submitCommandBuffers.data();
    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
    submitInfo.pSignalSemaphores = signalSemaphores.data();

    VkQueue graphicsQueue;
    m_VulkanContext->GetQueue(graphicsQueue, QueueIndex::eGraphics);
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

    VkQueue presentQueue;
    m_VulkanContext->GetQueue(presentQueue, QueueIndex::ePresent);

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

    delete m_ImGuiPipeline;
    delete m_ViewportPipeline;

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

    m_VulkanSwapchain->CleanupAll();
    m_VulkanContext->CleanupAll();

    glfwDestroyWindow(m_Window);
    glfwTerminate();

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

    m_VulkanSwapchain->CleanupImageViews();
    m_VulkanSwapchain->CleanupSwapchain();

    CHK_RESULT(m_VulkanSwapchain->CreateSwapchain(m_Window),
        "Swapchain was not created!");

    CHK_RESULT(m_VulkanSwapchain->CreateImages(m_ImGuiPipeline->GetVulkanFrameBuffer()),
        "Swapchain was not created!");

    CHK_RESULT(m_VulkanSwapchain->CreateImageViews(VK_IMAGE_ASPECT_COLOR_BIT), 
        "Image views were not created!");

    m_ViewportPipeline->RecreateFrameBuffer();

    m_ImGuiPipeline->RecreateFrameBuffer();
    m_ImGuiPipeline->RecreateDescriptorSets(m_ViewportPipeline->GetImages());
}

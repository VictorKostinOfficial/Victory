#include <iostream>

#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>

#include <vector>
#include <string>

#include "vulkan_renderer/Window.h"
#include "vulkan_renderer/VulkanDevice.h"
#include "vulkan_renderer/VulkanSwapchain.h"
#include "vulkan_renderer/VulkanFrameBuffer.h"
#include "vulkan_renderer/VulkanImage.h"
#include "vulkan_renderer/VulkanGraphicsPipeline.h"
#include "vulkan_renderer/VulkanModel.h"
#include "vulkan_renderer/VulkanUtils.h"

#include "../Victory/src/Utils.h"
#include "../Victory/src/renderer/vulkan_renderer/VulkanVertexData.h"

#include "../Victory/externs/imgui/backends/imgui_impl_glfw.h"
#include "../Victory/externs/imgui/backends/imgui_impl_vulkan.h"
#include <backends/imgui_impl_vulkan.h>

const static std::string s_ApplicationName{ "Victory Application" };
const static std::string s_EngineName{ "Victory Engine" };
static VkExtent2D s_ViewportSize{ 1080, 720 };
static std::vector<Victory::VulkanModel> models;

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

// Uniform Buffer
#include <chrono>

class Layer {
public:

    Layer() = default;
	virtual ~Layer() = default;

	virtual void OnAttach() {}
	virtual void OnDetach() {}

    virtual void OnEvent() {}

	virtual void OnUpdate() {}

private:

    bool m_BlockEvents = true;
};

namespace Victory 
{
    void check_vk_result(VkResult err)
    {
	    if (err == 0)
		    return;
	    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
	    if (err < 0)
		    abort();
    }

    class ViewportPipeline : public VulkanGraphicsPipeline 
    {
    public:

        ViewportPipeline() {};

        virtual ~ViewportPipeline() override 
        {
            VkDevice device{ m_VulkanDevice->GetDevice() };

            vkFreeMemory(device, m_UniformBufferMemory, nullptr);
            vkDestroyBuffer(device, m_UniformBuffer, nullptr);

            m_FrameBuffer->CleanupAll();
            delete m_FrameBuffer;

            vkDestroyPipeline(device, m_Pipeline, nullptr);
            vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
            vkDestroyPipelineCache(device, m_PipelineCache, nullptr);
            vkDestroyRenderPass(device, m_RenderPass, nullptr);
            vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
        }

        virtual void InitResources(VkFormat format_, VkExtent2D extent_, 
            const uint32_t frameBuffersCount_) override
        {
            m_FramesImageCI.pNext = nullptr;
            m_FramesImageCI.flags = 0;
            m_FramesImageCI.imageType = VK_IMAGE_TYPE_2D;
            m_FramesImageCI.format = format_;
            m_FramesImageCI.extent.depth = 1;
            m_FramesImageCI.extent.height = s_ViewportSize.height;
            m_FramesImageCI.extent.width = s_ViewportSize.width;
            m_FramesImageCI.mipLevels = 1;
            m_FramesImageCI.arrayLayers = 1;
            m_FramesImageCI.samples = VK_SAMPLE_COUNT_1_BIT;
            m_FramesImageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
            m_FramesImageCI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            m_FramesImageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            m_FramesImageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            m_VulkanDevice = Victory::VulkanDevice::Init();
            m_VulkanSwapchain = Victory::VulkanSwapchain::Init();

            CreateDescriptorSetLayout();

            CreateRenderPass();
            CreatePipelineCash();
            CreatePipelineLayout();
            CreatePipeline();

            CreateUniformBuffer();

            CreateFrameBuffers(frameBuffersCount_);
        }

        virtual VkCommandBuffer BeginFrame(const uint32_t currentFrame_) override 
        {
            UpdateUniformBuffer();

            VkCommandBufferBeginInfo beginI{};
            beginI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginI.pNext = nullptr;
            beginI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            beginI.pInheritanceInfo = nullptr;

            m_CurrentCommandBuffer = m_FrameBuffer->GetCommandBuffer(currentFrame_);
            vkBeginCommandBuffer(m_CurrentCommandBuffer, &beginI);

            return m_CurrentCommandBuffer;
        }

        virtual void RecordBuffer(const uint32_t bufferIndex_) override 
        {
            std::array<VkClearValue, 2> clearValues{};
            clearValues[0].color = {{0.f, 0.f, 0.f, 1.f}};
            clearValues[1].depthStencil = {1.f, 0};

            VkRenderPassBeginInfo renderPassBI{};
            renderPassBI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBI.pNext = nullptr;
            renderPassBI.renderPass = m_RenderPass;
            renderPassBI.framebuffer = m_FrameBuffer->GetFrameBuffer(bufferIndex_);
            renderPassBI.renderArea.extent.height = m_FramesImageCI.extent.height;
            renderPassBI.renderArea.extent.width = m_FramesImageCI.extent.width;
            renderPassBI.renderArea.offset = { 0,0 };
            renderPassBI.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassBI.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(m_CurrentCommandBuffer, &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);
            {
                vkCmdBindPipeline(m_CurrentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

                VkViewport viewport{};
                viewport.x = 0.f;
                viewport.y = 0.f;
                viewport.width = static_cast<float>(m_FramesImageCI.extent.width);
                viewport.height = static_cast<float>(m_FramesImageCI.extent.height);
                viewport.minDepth = 0.f;
                viewport.maxDepth = 1.f;

                vkCmdSetViewport(m_CurrentCommandBuffer, 0, 1, &viewport);
                vkCmdSetScissor(m_CurrentCommandBuffer, 0, 1, &renderPassBI.renderArea);

                for (auto&& model : models)
                {
                    std::vector<VkDeviceSize> offsets{0};
                    vkCmdBindVertexBuffers(m_CurrentCommandBuffer, 0, 1, &model.GetVertexBuffer(), offsets.data());
                    vkCmdBindIndexBuffer(m_CurrentCommandBuffer, model.GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT16);

                    // TODO: descriptor set for every model
                    vkCmdBindDescriptorSets(m_CurrentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
                        m_PipelineLayout, 0, 1, &model.GetDescriptorSet(), 0, nullptr);

                    vkCmdDrawIndexed(m_CurrentCommandBuffer, static_cast<uint32_t>(model.GetIndices().size()), 1, 0, 0, 0);
                }
            }
            vkCmdEndRenderPass(m_CurrentCommandBuffer);
        }

        virtual void EndFrame() override 
        {
            vkEndCommandBuffer(m_CurrentCommandBuffer);
        }

        virtual void RecreateResources() override 
        {
            m_FrameBuffer->CleanupAll();
            CreateFrameBuffers(m_VulkanSwapchain->GetImageCount());
        };

        VkDescriptorSetLayout GetDescriptorSetLayout() const
        {
            return m_DescriptorSetLayout;
        }

        VkBuffer GetUniformBuffer() const 
        {
            return m_UniformBuffer;
        }

        const std::vector<VulkanImage>& GetImages() const
        {
            return m_FrameBuffer->GetFrameImages();
        }

    private:

        void CreateDescriptorSetLayout()
        {
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

            std::vector<VkDescriptorSetLayoutBinding>  bindings{uboLayoutBinding, samplerLayoutBinding};

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
            descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutCI.bindingCount = static_cast<uint32_t>(bindings.size());
            descriptorSetLayoutCI.pBindings = bindings.data();

            CheckVulkanResult(
                vkCreateDescriptorSetLayout(m_VulkanDevice->GetDevice(), 
                    &descriptorSetLayoutCI, nullptr, &m_DescriptorSetLayout),
                "Descriptor Set was not created");
        }

        void CreatePipelineLayout() 
        {
            VkPipelineLayoutCreateInfo pipelineLayoutCI{};
            pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutCI.setLayoutCount = 1;
            pipelineLayoutCI.pSetLayouts = &m_DescriptorSetLayout;
            pipelineLayoutCI.pushConstantRangeCount = 0;
            pipelineLayoutCI.pPushConstantRanges = nullptr;

            CheckVulkanResult(
                vkCreatePipelineLayout(m_VulkanDevice->GetDevice(), 
                    &pipelineLayoutCI, nullptr, &m_PipelineLayout),
                "Pipeline Layout was not created");
        }

        void CreateRenderPass() 
        {
            std::vector<VkAttachmentDescription> attachments{ 3 };
            // MSAA Attachment
            attachments[0].flags = 0;
            attachments[0].format = m_FramesImageCI.format;
            attachments[0].samples = m_VulkanDevice->GetMaxSampleCount();
            attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            // Depth Attachment
            attachments[1].flags = 0;
            attachments[1].format = m_VulkanDevice->FindDepthFormat();
            attachments[1].samples = m_VulkanDevice->GetMaxSampleCount();
            attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            // Resolve attachment
            attachments[2].flags = 0;
            attachments[2].format = m_FramesImageCI.format;
            attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
            attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[2].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkAttachmentReference msaaAttachmentRef{};
            msaaAttachmentRef.attachment = 0;
            msaaAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentReference depthAttachmentRef{};
            depthAttachmentRef.attachment = 1;
            depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkAttachmentReference colorAttachmentRef{};
            colorAttachmentRef.attachment = 2;
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            std::vector<VkSubpassDescription> subpassDescriptions{ 1 };
            subpassDescriptions[0].flags = 0;
            subpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpassDescriptions[0].inputAttachmentCount = 0;
            subpassDescriptions[0].pInputAttachments = nullptr;
            subpassDescriptions[0].colorAttachmentCount = 1;
            subpassDescriptions[0].pColorAttachments = &msaaAttachmentRef;
            subpassDescriptions[0].pResolveAttachments = &colorAttachmentRef;
            subpassDescriptions[0].pDepthStencilAttachment = &depthAttachmentRef;
            subpassDescriptions[0].preserveAttachmentCount = 0;
            subpassDescriptions[0].pPreserveAttachments = nullptr;

            std::vector<VkSubpassDependency> subpassDependencies{ 2 };
            // Before Begin Render Pass
	        subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	        subpassDependencies[0].dstSubpass = 0;
	        subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	        subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	        subpassDependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	        subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	        subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
            // After End Render Pass
	        subpassDependencies[1].srcSubpass = 0;
	        subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	        subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	        subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	        subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	        subpassDependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	        subpassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            VkRenderPassCreateInfo renderPassCI{};
            renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassCI.pNext = nullptr;
            renderPassCI.flags = 0;
            renderPassCI.attachmentCount = static_cast<uint32_t>(attachments.size());
            renderPassCI.pAttachments = attachments.data();
            renderPassCI.subpassCount = static_cast<uint32_t>(subpassDescriptions.size());
            renderPassCI.pSubpasses = subpassDescriptions.data();
            renderPassCI.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
            renderPassCI.pDependencies = subpassDependencies.data();

            CheckVulkanResult(
                vkCreateRenderPass(m_VulkanDevice->GetDevice(), &renderPassCI, nullptr, &m_RenderPass),
                "Render pass was not created");
        }

        void CreatePipelineCash()
        {
            VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	        pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
            CheckVulkanResult(
	            vkCreatePipelineCache(m_VulkanDevice->GetDevice(), 
                    &pipelineCacheCreateInfo, nullptr, &m_PipelineCache),
                "Pipeline Cash was not created");
        }

        void CreatePipeline()
        {
            const std::vector<char> vsBuffer = Utils::ReadFile("graphics.vert.spv");
            const std::vector<char> fsBuffer = Utils::ReadFile("graphics.frag.spv");

            VkShaderModule VS{ VK_NULL_HANDLE };
            VkShaderModule FS{ VK_NULL_HANDLE };

            CreateShaderModule(m_VulkanDevice->GetDevice(), vsBuffer, &VS);
            CreateShaderModule(m_VulkanDevice->GetDevice(), fsBuffer, &FS);

            std::vector<VkPipelineShaderStageCreateInfo> vertexShaderStageCIs{ 2 };
            vertexShaderStageCIs[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vertexShaderStageCIs[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
            vertexShaderStageCIs[0].module = VS;
            vertexShaderStageCIs[0].pName = "main";
            vertexShaderStageCIs[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vertexShaderStageCIs[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            vertexShaderStageCIs[1].module = FS;
            vertexShaderStageCIs[1].pName = "main";

            auto&& bindingDescription{ Victory::GetBindingDescription() };
            auto&& attributegDescriptions{ Victory::GetAttributeDescriptions() };

            VkPipelineVertexInputStateCreateInfo vertexInputStateCI{};
            vertexInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertexInputStateCI.pNext = nullptr;
            vertexInputStateCI.flags = 0;
            vertexInputStateCI.vertexBindingDescriptionCount = 1;
            vertexInputStateCI.pVertexBindingDescriptions = &bindingDescription;
            vertexInputStateCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributegDescriptions.size());
            vertexInputStateCI.pVertexAttributeDescriptions = attributegDescriptions.data();

            VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{};
            inputAssemblyStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            inputAssemblyStateCI.pNext = nullptr;
            inputAssemblyStateCI.flags = 0;
            inputAssemblyStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            inputAssemblyStateCI.primitiveRestartEnable = VK_FALSE;

            VkPipelineViewportStateCreateInfo viewportStateCI{};
            viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportStateCI.viewportCount = 1;
            viewportStateCI.scissorCount = 1;

            VkPipelineRasterizationStateCreateInfo rasterizationStateCI{};
            rasterizationStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizationStateCI.pNext = nullptr;
            rasterizationStateCI.flags = 0;
            rasterizationStateCI.depthClampEnable = VK_FALSE;
            rasterizationStateCI.rasterizerDiscardEnable = VK_FALSE;
            rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;
            rasterizationStateCI.cullMode = VK_CULL_MODE_BACK_BIT;
            rasterizationStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            rasterizationStateCI.depthBiasEnable = VK_FALSE;
            rasterizationStateCI.depthBiasConstantFactor = 0.f;
            rasterizationStateCI.depthBiasClamp = 0.f;
            rasterizationStateCI.depthBiasSlopeFactor = 0.f;
            rasterizationStateCI.lineWidth = 1.f;

            VkPipelineMultisampleStateCreateInfo multisamplingStateCI{};
            multisamplingStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisamplingStateCI.pNext = nullptr;
            multisamplingStateCI.flags = 0;
            multisamplingStateCI.rasterizationSamples = m_VulkanDevice->GetMaxSampleCount();
            multisamplingStateCI.sampleShadingEnable = VK_FALSE;
            multisamplingStateCI.minSampleShading = 1.f;
            multisamplingStateCI.pSampleMask = nullptr;
            multisamplingStateCI.alphaToCoverageEnable = VK_FALSE;
            multisamplingStateCI.alphaToOneEnable = VK_FALSE;

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
            colorBlendStateCI.pNext = nullptr;
            colorBlendStateCI.flags = 0;
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
                VK_DYNAMIC_STATE_SCISSOR };

            VkPipelineDynamicStateCreateInfo dynamicStateCI{};
            dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
            dynamicStateCI.pDynamicStates = dynamicStates.data();

            VkGraphicsPipelineCreateInfo pipelineCI{};
            pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineCI.pNext = nullptr;
            pipelineCI.flags = 0;
            pipelineCI.stageCount = static_cast<uint32_t>(vertexShaderStageCIs.size());
            pipelineCI.pStages = vertexShaderStageCIs.data();
            pipelineCI.pVertexInputState = &vertexInputStateCI;
            pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
            pipelineCI.pTessellationState = nullptr;
            pipelineCI.pViewportState = &viewportStateCI;
            pipelineCI.pRasterizationState = &rasterizationStateCI;
            pipelineCI.pMultisampleState = &multisamplingStateCI;
            pipelineCI.pDepthStencilState = &depthStencilStateCI;
            pipelineCI.pColorBlendState = &colorBlendStateCI;
            pipelineCI.pDynamicState = &dynamicStateCI;
            pipelineCI.layout = m_PipelineLayout;
            pipelineCI.renderPass = m_RenderPass;
            pipelineCI.subpass = 0;
            pipelineCI.basePipelineHandle = VK_NULL_HANDLE;
            pipelineCI.basePipelineIndex = -1;

            CheckVulkanResult(
                vkCreateGraphicsPipelines(m_VulkanDevice->GetDevice(), m_PipelineCache, 1, 
                    &pipelineCI, nullptr, &m_Pipeline),
                "Pipeline was not created");

            vkDestroyShaderModule(m_VulkanDevice->GetDevice(), FS, nullptr);
            vkDestroyShaderModule(m_VulkanDevice->GetDevice(), VS, nullptr);
        }

        void CreateFrameBuffers(const uint32_t frameBuffersCount)
        {
            m_FrameBuffer = new VulkanFrameBuffer(m_VulkanDevice);

            m_FramesImageCI.extent.width = s_ViewportSize.width;
            m_FramesImageCI.extent.height = s_ViewportSize.height;

            // Color Images
            m_FrameBuffer->CreateFrameBufferImages(m_FramesImageCI, frameBuffersCount);

            auto&& images{ m_FrameBuffer->GetFrameImages() };
            for (auto&& image : images)
            {
                image.TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }

            // Depth Image
            VkImageCreateInfo depthImageCI{};
            depthImageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            depthImageCI.pNext = nullptr;
            depthImageCI.flags = 0;
            depthImageCI.imageType = VK_IMAGE_TYPE_2D;
            depthImageCI.format = m_VulkanDevice->FindDepthFormat();
            depthImageCI.extent.depth = 1;
            depthImageCI.extent.height = s_ViewportSize.height;
            depthImageCI.extent.width = s_ViewportSize.width;
            depthImageCI.mipLevels = 1;
            depthImageCI.arrayLayers = 1;
            depthImageCI.samples = m_VulkanDevice->GetMaxSampleCount();
            depthImageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
            depthImageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            depthImageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            depthImageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            // MSAA
            VkImageCreateInfo msaaImageCI{};
            msaaImageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            msaaImageCI.pNext = nullptr;
            msaaImageCI.flags = 0;
            msaaImageCI.imageType = VK_IMAGE_TYPE_2D;
            msaaImageCI.format = m_VulkanSwapchain->GetSurfaceFormat().format;
            msaaImageCI.extent.depth = 1;
            msaaImageCI.extent.height = s_ViewportSize.height;
            msaaImageCI.extent.width = s_ViewportSize.width;
            msaaImageCI.mipLevels = 1;
            msaaImageCI.arrayLayers = 1;
            msaaImageCI.samples = m_VulkanDevice->GetMaxSampleCount();
            msaaImageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
            msaaImageCI.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            msaaImageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            msaaImageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            std::vector<VulkanImage> attachments(2, VulkanImage(m_VulkanDevice));
            attachments[0].CreateImage(msaaImageCI, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            attachments[0].CreateImageView(msaaImageCI.format, VK_IMAGE_ASPECT_COLOR_BIT);
            attachments[1].CreateImage(depthImageCI, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            attachments[1].CreateImageView(depthImageCI.format, VK_IMAGE_ASPECT_DEPTH_BIT);

            m_FrameBuffer->SetAttachments(attachments);

            m_FrameBuffer->CreateCommandPool(QueueIndex::eGraphics);
            m_FrameBuffer->CreateFrameBuffers(m_RenderPass, s_ViewportSize);
            m_FrameBuffer->CreateCommandBuffers();
        }

        void CreateUniformBuffer() 
        {
            VkDeviceSize bufferSize = static_cast<uint64_t>(sizeof(UniformBufferObject));

            VkDevice device{ m_VulkanDevice->GetDevice() };

            VkBufferCreateInfo bufferCI{};
            bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferCI.size = bufferSize;
            bufferCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            vkCreateBuffer(device, &bufferCI, nullptr, &m_UniformBuffer);

            VkMemoryRequirements memRequirements{};
            vkGetBufferMemoryRequirements(device, m_UniformBuffer, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = m_VulkanDevice->FindMemoryType(memRequirements.memoryTypeBits, 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            vkAllocateMemory(device, &allocInfo, nullptr, &m_UniformBufferMemory);

            vkBindBufferMemory(device, m_UniformBuffer, m_UniformBufferMemory, 0);

            vkMapMemory(device, m_UniformBufferMemory, 0, bufferSize, 0, &m_UniformBufferMapped);
        }

        void UpdateUniformBuffer() 
        {
            static auto startTime = std::chrono::high_resolution_clock::now();

            auto currentTime = std::chrono::high_resolution_clock::now();
            float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

            UniformBufferObject ubo{};
            ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(10.0f), glm::vec3(0.f, 0.f, 1.f));
            ubo.view = glm::lookAt(glm::vec3(2.f, 2.f, 2.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 1.f));
            ubo.proj = glm::perspective(glm::radians(45.0f), 
                m_FramesImageCI.extent.width / static_cast<float>(m_FramesImageCI.extent.height), 0.1f, 10.0f);
            ubo.proj[1][1] *= -1;

            memcpy(m_UniformBufferMapped, &ubo, sizeof(ubo));
        }
    
    private:

        VkBuffer m_UniformBuffer;
        VkDeviceMemory m_UniformBufferMemory;
        void* m_UniformBufferMapped;
    };

    class ImGuiPipeline : public VulkanGraphicsPipeline
    {
    public:
        ImGuiPipeline() {};
        virtual ~ImGuiPipeline() 
        {
            // CleanupDescriptorSets();
            // vkDestroySampler(m_Context->GetDevice(), m_ViewportSampler, nullptr);
            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();

            m_FrameBuffer->CleanupAll();
            delete m_FrameBuffer;

            VkDevice device{ m_VulkanDevice->GetDevice() };
            vkDestroySampler(device, m_Sampler, nullptr);
            vkDestroyPipeline(device, m_Pipeline, nullptr);
            vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
            vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
            vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
            vkDestroyRenderPass(device, m_RenderPass, nullptr);
        };

        virtual void InitResources(VkFormat format_, 
            VkExtent2D extent_, const uint32_t frameBuffersCount_)
        {
            m_VulkanDevice = Victory::VulkanDevice::Init();
            m_VulkanSwapchain = Victory::VulkanSwapchain::Init();
            m_Window = Victory::Window::Init(nullptr);

            CreateImGuiContext();
            CreateRenderPass();
            CreateDescriptorPool();
            CreateDescriptorSetLayout();
            CreatePipelineLayout();
            CreatePipeline();
            InitImGui(frameBuffersCount_);
            CreateFrameBuffers();
        }

        virtual VkCommandBuffer BeginFrame(const uint32_t currentFrame_)
        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();

            ImGuiIO &io = ImGui::GetIO();
            ImGui::NewFrame();
            {
                static bool dockspaceOpen = true;
		        static bool opt_fullscreen_persistant = true;
		        bool opt_fullscreen = opt_fullscreen_persistant;

		        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
                ImGuiWindowFlags window_flags{ ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking };

                if (opt_fullscreen) {
			        ImGuiViewport* viewport = ImGui::GetMainViewport();
			        ImGui::SetNextWindowPos(viewport->Pos);
			        ImGui::SetNextWindowSize(viewport->Size);
			        ImGui::SetNextWindowViewport(viewport->ID);
			        // ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			        // ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		        }

                if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			        window_flags |= ImGuiWindowFlags_NoBackground;

                ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
                {
                    ImGuiIO& io = ImGui::GetIO();
		            if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
			            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		            }
                }
                ImGui::End();

                ImGui::ShowDemoWindow();
                ImGui::ShowMetricsWindow();
        
                ImGui::Begin("Viewport");
                {
		            ImVec2 viewportPanelSize{ ImGui::GetContentRegionAvail() };
                    if (s_ViewportSize.height != viewportPanelSize.y ||
                        s_ViewportSize.width != viewportPanelSize.x)
                    {
                        s_ViewportSize.height = static_cast<uint32_t>(viewportPanelSize.y);
                        s_ViewportSize.width = static_cast<uint32_t>(viewportPanelSize.x);
                        m_NeedResize = true;
                    }
                    ImGui::Image(m_DescriptorSets[currentFrame_], ImVec2{ viewportPanelSize.x, viewportPanelSize.y });
                }
		        ImGui::End();
            }
            ImGui::Render();
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }

            VkCommandBufferBeginInfo beginI{};
            beginI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginI.pNext = nullptr;
            beginI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            beginI.pInheritanceInfo = nullptr;

            m_CurrentCommandBuffer = m_FrameBuffer->GetCommandBuffer(currentFrame_);
            vkBeginCommandBuffer(m_CurrentCommandBuffer, &beginI);

            return m_CurrentCommandBuffer;
        }

        virtual void RecordBuffer(const uint32_t imageIndex_)
        {
            VkClearValue clearColor = {0.f, 0.f, 0.f, 0.f};

            VkRenderPassBeginInfo renderPassBI{};
            renderPassBI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBI.renderPass = m_RenderPass;
            renderPassBI.framebuffer = m_FrameBuffer->GetFrameBuffer(imageIndex_);
            renderPassBI.renderArea.offset = {0, 0};
            renderPassBI.renderArea.extent = m_VulkanSwapchain->GetExtent();
            renderPassBI.clearValueCount = 1;
            renderPassBI.pClearValues = &clearColor;

            vkCmdBeginRenderPass(m_CurrentCommandBuffer, &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);
            {
                ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_CurrentCommandBuffer);
            }
            vkCmdEndRenderPass(m_CurrentCommandBuffer);
        }

        virtual void EndFrame()
        {
            vkEndCommandBuffer(m_CurrentCommandBuffer);
        }

        virtual void RecreateResources()
        {
            m_FrameBuffer->CleanupAll();
            CreateFrameBuffers();
        }

        void InitDescriptorSets(const std::vector<VulkanImage>& images_, bool needCreateSampler = false) 
        {
            if (needCreateSampler)
            {
                CreateSampler();
            }

            const size_t imageCount = images_.size();
            m_DescriptorSets.resize(imageCount);
            for (size_t i{ 0 }; i < imageCount; ++i) 
            {
                m_DescriptorSets[i] = ImGui_ImplVulkan_AddTexture(m_Sampler, 
                    images_[i].GetImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }
        }

        inline void SetNeedResize(bool value_)
        {
            m_NeedResize = value_;
        }

        inline bool GetNeedResize() const
        {
            return m_NeedResize;
        }

    private:

        void CreateSampler() 
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

	        vkCreateSampler(m_VulkanDevice->GetDevice(), &samplerInfo, nullptr, &m_Sampler);
        }

        void CreateImGuiContext()
        {
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
        }

        void InitImGui(const uint32_t minImageCount_) 
        {

            ImGui_ImplGlfw_InitForVulkan(m_Window, true);
            ImGui_ImplVulkan_InitInfo info;
            info.Instance = m_VulkanDevice->GetInstance();
            info.PhysicalDevice = m_VulkanDevice->GetPhysicalDevice();
            info.Device = m_VulkanDevice->GetDevice();
            info.QueueFamily = m_VulkanDevice->GetQueueIndex(QueueIndex::eGraphics);
            m_VulkanDevice->GetQueue(info.Queue, QueueIndex::eGraphics);
            info.PipelineCache = 0;
            info.DescriptorPool = m_DescriptorPool;
            info.Subpass = 0;
            info.MinImageCount = minImageCount_;
            info.ImageCount = m_VulkanSwapchain->GetImageCount();
            info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
            info.UseDynamicRendering = false;
            info.Allocator = 0;
            info.CheckVkResultFn = check_vk_result;
            ImGui_ImplVulkan_Init(&info, m_RenderPass);
            ImGui_ImplVulkan_CreateFontsTexture();
        }

        void CreateRenderPass() {
            std::array<VkAttachmentDescription, 1> attachmets({});
            attachmets[0].format = m_VulkanSwapchain->GetSurfaceFormat().format;
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

            CheckVulkanResult(
	            vkCreateRenderPass(m_VulkanDevice->GetDevice(), &info, nullptr, &m_RenderPass),
		        "Failed to create render pass!");
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

            CheckVulkanResult(
	            vkCreateDescriptorPool(m_VulkanDevice->GetDevice(), &pool_info, nullptr, &m_DescriptorPool),
		        "Failed to create descriptor pool!");
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

            vkCreateDescriptorSetLayout(m_VulkanDevice->GetDevice(), &descriptorSetLayoutCI, 
                nullptr, &m_DescriptorSetLayout);
        }

        void CreatePipelineLayout() {
            VkPipelineLayoutCreateInfo pipelineLayoutCI{};
            pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutCI.setLayoutCount = 1;
            pipelineLayoutCI.pSetLayouts = &m_DescriptorSetLayout;
            pipelineLayoutCI.pushConstantRangeCount = 0;
            pipelineLayoutCI.pPushConstantRanges = nullptr;

            vkCreatePipelineLayout(m_VulkanDevice->GetDevice(), &pipelineLayoutCI, 
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

            VkShaderModule VS{ VK_NULL_HANDLE };
            VkShaderModule FS{ VK_NULL_HANDLE };

            CreateShaderModule(m_VulkanDevice->GetDevice(), vsBuffer, &VS);
            CreateShaderModule(m_VulkanDevice->GetDevice(), fsBuffer, &FS);

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

            CheckVulkanResult(
                vkCreateGraphicsPipelines(m_VulkanDevice->GetDevice(), nullptr, 1, 
                    &pipelineCI, nullptr, &m_Pipeline), 
                "Pipeline was not created");

            vkDestroyShaderModule(m_VulkanDevice->GetDevice(), FS, nullptr);
            vkDestroyShaderModule(m_VulkanDevice->GetDevice(), VS, nullptr);
        }

        void CreateFrameBuffers()
        {
            m_FrameBuffer = new VulkanFrameBuffer(m_VulkanDevice);

            // Color Image
            uint32_t imageCount{ m_VulkanSwapchain->GetImageCount() };
            std::vector<VkImage> images;
            images.resize(imageCount);
            vkGetSwapchainImagesKHR(m_VulkanDevice->GetDevice(), 
                m_VulkanSwapchain->GetSwapchain(), &imageCount, images.data());

            m_FrameBuffer->AttachFrameBufferImages(images, m_VulkanSwapchain->GetSurfaceFormat().format);
            m_FrameBuffer->CreateCommandPool(QueueIndex::eGraphics);
            m_FrameBuffer->CreateFrameBuffers(m_RenderPass, m_VulkanSwapchain->GetExtent());
            m_FrameBuffer->CreateCommandBuffers();
            m_ViewportExtent = m_VulkanSwapchain->GetExtent();
        }

    private:

        GLFWwindow* m_Window;

        VkDescriptorPool m_DescriptorPool;
        std::vector<VkDescriptorSet> m_DescriptorSets;

        VkSampler m_Sampler;

        VkExtent2D m_ViewportExtent;
        bool m_NeedResize{ false };
    };
}

void OnWindowResize(GLFWwindow *window_, int width_, int height_);
void OnWindowClose(GLFWwindow *window_);

class MiniRenderer {
public:

    void Entry() {
        m_Window = Victory::Window::Init(this);
        Victory::Window::SetResizeCallback(OnWindowResize);
        Victory::Window::SetCloseCallback(OnWindowClose);

        m_VulkanDevice = Victory::VulkanDevice::Init();
        m_VulkanDevice->CreateInstance();
        m_VulkanSwapchain = Victory::VulkanSwapchain::Init(m_VulkanDevice, m_Window);
        m_VulkanSwapchain->CreateSurface();
        m_VulkanDevice->PickPhysicalDevice(m_VulkanSwapchain->GetSurface());
        m_VulkanDevice->CreateLogicalDevice();
        m_VulkanSwapchain->CreateSwapchain();

        CreateSemaphores();

        // TODO: Map where key is enum like Viewport, ImGui, etc.
        std::vector<Victory::VulkanGraphicsPipeline*> pipelines;
        Victory::ImGuiPipeline* ImGuiPipeline{ new Victory::ImGuiPipeline() };
        Victory::ViewportPipeline* ViewportPipeline{ new Victory::ViewportPipeline() };
        pipelines.emplace_back(ImGuiPipeline);
        pipelines.emplace_back(ViewportPipeline);

        for (auto&& pipeline : pipelines)
        {
            pipeline->InitResources(m_VulkanSwapchain->GetSurfaceFormat().format, 
                m_VulkanSwapchain->GetExtent(), m_VulkanSwapchain->GetImageCount());
        }

        ImGuiPipeline->InitDescriptorSets(ViewportPipeline->GetImages(), true);

        VkImageCreateInfo imageCI{};
        imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCI.pNext = nullptr;
        imageCI.flags = 0;
        imageCI.imageType = VK_IMAGE_TYPE_2D;
        imageCI.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageCI.extent.width;
        imageCI.extent.height;
        imageCI.extent.depth = 1;
        imageCI.mipLevels = 1;
        imageCI.arrayLayers = 1;
        imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCI.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        // imageCI.queueFamilyIndexCount = 1;
        // imageCI.pQueueFamilyIndices = &s_QueueIndex;
        imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        Victory::VulkanModel newModel;
        newModel.LoadModel("viking_room.obj");
        newModel.LoadTexture("viking_room.png", imageCI);

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = ViewportPipeline->GetUniformBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);
        newModel.CreateDescriptors(ViewportPipeline->GetDescriptorSetLayout(), bufferInfo);
        models.emplace_back(newModel);

        while (m_IsRunning)
        {
            Victory::Window::PollEvents();

            VkDevice device{ m_VulkanDevice->GetDevice() };

            // Resize
            uint32_t imageIndex;
            {
                vkWaitForFences(device, 1, &m_QueueSubmitFence[m_CurrentFrame], VK_TRUE, UINT64_MAX);

                VkResult acquireResult = vkAcquireNextImageKHR(device, m_VulkanSwapchain->GetSwapchain(), UINT64_MAX, 
                    m_ImageAvailableSemaphore[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);

                if (m_IsResized || 
                    acquireResult == VK_ERROR_OUT_OF_DATE_KHR || 
                    acquireResult == VK_SUBOPTIMAL_KHR) 
                {
                    while (m_WindowWidth == 0 || m_WindowHeight == 0) 
                    {
                        Victory::Window::WaitEvents();
                    }
                    m_VulkanSwapchain->RecreateSwapchain();
                    for (auto&& pipeline : pipelines)
                    {
                        pipeline->RecreateResources();
                    }
                    m_IsResized = false;
                    ImGuiPipeline->SetNeedResize(false);
                    ImGuiPipeline->InitDescriptorSets(ViewportPipeline->GetImages());

                    vkDestroySemaphore(device, m_ImageAvailableSemaphore[m_CurrentFrame], nullptr);

                    VkSemaphoreCreateInfo semaphoreCI{};
                    semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
                    vkCreateSemaphore(device, &semaphoreCI, nullptr, &m_ImageAvailableSemaphore[m_CurrentFrame]);
                    continue;
                }

                if (ImGuiPipeline->GetNeedResize())
                {
                    vkDeviceWaitIdle(device);
                    ViewportPipeline->RecreateResources();
                    ImGuiPipeline->SetNeedResize(false);
                    ImGuiPipeline->InitDescriptorSets(ViewportPipeline->GetImages());
                }
            }

            vkResetFences(device, 1, &m_QueueSubmitFence[m_CurrentFrame]);

            std::vector<VkCommandBuffer> commandBuffers;
            commandBuffers.reserve(pipelines.size());
            for (auto&& pipeline : pipelines)
            {
                commandBuffers.emplace_back(pipeline->BeginFrame(m_CurrentFrame));
                pipeline->RecordBuffer(imageIndex);
                pipeline->EndFrame();
            }

            // Submit
            {
                VkPipelineStageFlags waitFlag{ VK_PIPELINE_STAGE_TRANSFER_BIT };
                VkSubmitInfo submitI{};
                submitI.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitI.pNext = nullptr;
                submitI.waitSemaphoreCount = 1;
                submitI.pWaitSemaphores = &m_ImageAvailableSemaphore[m_CurrentFrame];
                submitI.pWaitDstStageMask = &waitFlag;
                submitI.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
                submitI.pCommandBuffers = commandBuffers.data();
                submitI.signalSemaphoreCount = 1;
                submitI.pSignalSemaphores = &m_RenderingFinishedSemaphore[m_CurrentFrame];

                VkQueue queue;
                m_VulkanDevice->GetQueue(queue, Victory::QueueIndex::eGraphics);
                vkQueueSubmit(queue, 1, &submitI, m_QueueSubmitFence[m_CurrentFrame]);

                const std::vector<VkSwapchainKHR> swapOld{m_VulkanSwapchain->GetSwapchain()};
                VkPresentInfoKHR presentI{};
                presentI.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
                presentI.pNext = nullptr;
                presentI.waitSemaphoreCount = 1;
                presentI.pWaitSemaphores = &m_RenderingFinishedSemaphore[m_CurrentFrame];
                presentI.swapchainCount = static_cast<uint32_t>(swapOld.size());
                presentI.pSwapchains = swapOld.data();
                presentI.pImageIndices = &imageIndex;
                presentI.pResults = nullptr;

                vkQueuePresentKHR(queue, &presentI);
            }

            m_CurrentFrame = (m_CurrentFrame + 1) % m_MaxImageInFight;
        }

        // Destroy
        {
            vkDeviceWaitIdle(m_VulkanDevice->GetDevice());

            models.clear();

            for (auto&& pipeline : pipelines)
            {
                delete pipeline;
            }

            CleanupSemaphores();
            newModel.CleanupAll();
            Victory::VulkanSwapchain::Cleanup();
            Victory::VulkanDevice::Cleanup();
            Victory::Window::Cleanup();
        }
    }

    void CreateSemaphores() {
        VkSemaphoreCreateInfo semaphoreCI{};
        semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreCI.pNext = nullptr;
        semaphoreCI.flags = 0;

        VkFenceCreateInfo fenceCI{};
        fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCI.pNext = nullptr;
        fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        m_ImageAvailableSemaphore.resize(m_MaxImageInFight);
        m_RenderingFinishedSemaphore.resize(m_MaxImageInFight);
        m_QueueSubmitFence.resize(m_MaxImageInFight);

        VkDevice device{ m_VulkanDevice->GetDevice() };
        for (uint32_t i{ 0 }; i < m_MaxImageInFight; ++i)
        {
            vkCreateSemaphore(device, &semaphoreCI, nullptr, &m_ImageAvailableSemaphore[i]);
            vkCreateSemaphore(device, &semaphoreCI, nullptr, &m_RenderingFinishedSemaphore[i]);
            vkCreateFence(device, &fenceCI, nullptr, &m_QueueSubmitFence[i]);
        }
    }

    void CleanupSemaphores() {
        VkDevice device{ m_VulkanDevice->GetDevice() };
        for (uint32_t i{ 0 }; i < m_MaxImageInFight; ++i) {
            vkDestroyFence(device, m_QueueSubmitFence[i], nullptr);
            vkDestroySemaphore(device, m_ImageAvailableSemaphore[i], nullptr);
            vkDestroySemaphore(device, m_RenderingFinishedSemaphore[i], nullptr);
        }
    }

    void SetIsResized(bool value, int width_, int height_) {
        std::cout << "Window is resized" << std::endl;
        m_WindowWidth = width_;
        m_WindowHeight = height_;
        m_IsResized = value;
    }

    void SetIsRunning(bool value) {
        std::cout << "Window is closed" << std::endl;
        m_IsRunning = value;
    }

private:

    GLFWwindow* m_Window;
    int m_WindowWidth{ 0 };
    int m_WindowHeight{ 0 };

    Victory::VulkanDevice* m_VulkanDevice;
    Victory::VulkanSwapchain* m_VulkanSwapchain;

    bool m_IsRunning{ true };
    bool m_IsResized{ false };

    const uint32_t m_MaxImageInFight{ 2 };
    uint32_t m_CurrentFrame{ 0 };
    
    std::vector<VkSemaphore> m_ImageAvailableSemaphore;
    std::vector<VkSemaphore> m_RenderingFinishedSemaphore;
    std::vector<VkFence> m_QueueSubmitFence;
};

void OnWindowResize(GLFWwindow *window_, int width_, int height_) {
    auto app = reinterpret_cast<MiniRenderer*>(glfwGetWindowUserPointer(window_));
    app->SetIsResized(true, width_, height_);
}

void OnWindowClose(GLFWwindow *window_) {
    auto app = reinterpret_cast<MiniRenderer*>(glfwGetWindowUserPointer(window_));
    app->SetIsRunning(false);
}

int main()
{
    MiniRenderer entry;
    entry.Entry();
}
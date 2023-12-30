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
#include "vulkan_renderer/VulkanUtils.h"

#include "../Victory/src/Utils.h"
#include "../Victory/src/renderer/vulkan_renderer/VulkanVertexData.h"

const static std::string ApplicationName{ "Testing features application" };
const static std::string EngineName{ "Testing Engine" };

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

// Load Object
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

static std::vector<VertexData> s_Vertices;
static std::vector<uint16_t> s_Indices;

static VkBuffer s_VertexBuffer{VK_NULL_HANDLE};
static VkDeviceMemory s_VertexBufferMemory{VK_NULL_HANDLE};
static VkBuffer s_IndexBuffer{VK_NULL_HANDLE};
static VkDeviceMemory s_IndexBufferMemory{VK_NULL_HANDLE};

// Load Image
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

static VkImage s_ObjectImage{ VK_NULL_HANDLE }; 
static VkImageView s_ObjectImageView{ VK_NULL_HANDLE };
static VkSampler s_ObjectImageSampler{ VK_NULL_HANDLE };
static VkDeviceMemory s_ObjectImageDeviceMemory{ VK_NULL_HANDLE };

// Uniform Buffer
#include <chrono>

VkDescriptorPool s_DescriptorPool{VK_NULL_HANDLE};
VkDescriptorSet s_DescriptorSet;

VkBuffer s_UniformBuffer;
VkDeviceMemory s_UniformBufferMemory;
void* s_UniformBufferMapped;

struct CreateBufferSettings {
    VkDeviceSize size;
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags properties;
};

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

    class ViewportPipeline : public VulkanGraphicsPipeline 
    {
    public:

        ViewportPipeline() {};

        virtual ~ViewportPipeline() override 
        {
            VkDevice device{ m_VulkanDevice->GetDevice() };

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
            m_FramesImageCI.extent.height = extent_.height;
            m_FramesImageCI.extent.width = extent_.width;
            m_FramesImageCI.mipLevels = 1;
            m_FramesImageCI.arrayLayers = 1;
            m_FramesImageCI.samples = VK_SAMPLE_COUNT_1_BIT;
            m_FramesImageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
            m_FramesImageCI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            m_FramesImageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            // m_FramesImageCI.queueFamilyIndexCount = 1;
            // m_FramesImageCI.pQueueFamilyIndices = &s_QueueIndex;
            m_FramesImageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            m_VulkanDevice = Victory::VulkanDevice::Init();
            m_VulkanSwapchain = Victory::VulkanSwapchain::Init();

            CreateDescriptorSetLayout();

            CreateRenderPass();
            CreatePipelineCash();
            CreatePipelineLayout();
            CreatePipeline();

            CreateFrameBuffers(frameBuffersCount_);
        }

        virtual VkCommandBuffer BeginFrame(const uint32_t currentFrame_) override 
        {
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
            clearValues[0].color = {{0.f, 1.f, 0.f, 1.f}};
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

                // std::vector<VkDeviceSize> offsets{0};

                // vkCmdBindVertexBuffers(m_CurrentCommandBuffer, 0, 1, &s_VertexBuffer, offsets.data());
                // vkCmdBindIndexBuffer(m_CurrentCommandBuffer, s_IndexBuffer, 0, VK_INDEX_TYPE_UINT16);

                // vkCmdBindDescriptorSets(m_CurrentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
                //     m_PipelineLayout, 0, 1, &s_DescriptorSet, 0, nullptr);

                // vkCmdDrawIndexed(m_CurrentCommandBuffer, static_cast<uint32_t>(s_Indices.size()), 1, 0, 0, 0);
            }
            vkCmdEndRenderPass(m_CurrentCommandBuffer);
        }

        virtual void EndFrame() override 
        {
            vkEndCommandBuffer(m_CurrentCommandBuffer);
        }

        virtual void RecreateResources() override 
        {
        };

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
            std::vector<VkAttachmentDescription> attachments{ 2 };
            // Color attachment
            attachments[0].flags = 0;
            attachments[0].format = m_FramesImageCI.format;
            attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
            attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            // Depth Attachment
            attachments[1].flags = 0;
            attachments[1].format = m_VulkanDevice->FindDepthFormat();
            attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
            attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkAttachmentReference colorAttachmentRef{};
            colorAttachmentRef.attachment = 0;
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentReference depthAttachmentRef{};
            depthAttachmentRef.attachment = 1;
            depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            std::vector<VkSubpassDescription> subpassDescriptions{ 1 };
            subpassDescriptions[0].flags = 0;
            subpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpassDescriptions[0].inputAttachmentCount = 0;
            subpassDescriptions[0].pInputAttachments = nullptr;
            subpassDescriptions[0].colorAttachmentCount = 1;
            subpassDescriptions[0].pColorAttachments = &colorAttachmentRef;
            subpassDescriptions[0].pResolveAttachments = nullptr;
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
            std::vector<char> vsBuffer = Utils::ReadFile("graphics.vert.spv");
            std::vector<char> fsBuffer = Utils::ReadFile("graphics.frag.spv");

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
            multisamplingStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
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

            // Color Image
            m_FrameBuffer->CreateFrameBufferImages(m_FramesImageCI, frameBuffersCount);

            // Depth Image
            VkImageCreateInfo imageCI{};
            imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageCI.pNext = nullptr;
            imageCI.flags = 0;
            imageCI.imageType = VK_IMAGE_TYPE_2D;
            imageCI.format = m_VulkanDevice->FindDepthFormat();
            imageCI.extent = m_FramesImageCI.extent;
            imageCI.mipLevels = 1;
            imageCI.arrayLayers = 1;
            imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            // imageCI.queueFamilyIndexCount = 1;
            // imageCI.pQueueFamilyIndices = &s_QueueIndex;
            imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            std::vector<VulkanImage> attachments(1, VulkanImage(m_VulkanDevice));
            attachments[0].CreateImage(imageCI, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            attachments[0].CreateImageView(imageCI.format, VK_IMAGE_ASPECT_DEPTH_BIT);

            m_FrameBuffer->SetAttachments(attachments);

            m_FrameBuffer->CreateCommandPool(QueueIndex::eGraphics);
            VkExtent2D extent{ m_FramesImageCI.extent.width, m_FramesImageCI.extent.height };
            m_FrameBuffer->CreateFrameBuffers(m_RenderPass, extent);
            m_FrameBuffer->CreateCommandBuffers();
        }

    private:
    };

    class ImGuiPipeline : public VulkanGraphicsPipeline
    {
    public:
        ImGuiPipeline() {};
        virtual ~ImGuiPipeline() 
        {
            m_FrameBuffer->CleanupAll();
            delete m_FrameBuffer;
        };

        virtual void InitResources(VkFormat format_, 
            VkExtent2D extent_, const uint32_t frameBuffersCount_)
        {
            m_VulkanDevice = Victory::VulkanDevice::Init();
            m_VulkanSwapchain = Victory::VulkanSwapchain::Init();

            CreateFrameBuffers();
        }

        virtual VkCommandBuffer BeginFrame(const uint32_t currentFrame_)
        {
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
            VkImageSubresourceRange imageSubresource{};
            imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageSubresource.baseMipLevel = 0;
            imageSubresource.levelCount = 1;
            imageSubresource.baseArrayLayer = 0;
            imageSubresource.layerCount = 1;

            VkImageMemoryBarrier barrierFromClearToPresent{};
            barrierFromClearToPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrierFromClearToPresent.pNext = nullptr;
            barrierFromClearToPresent.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
            barrierFromClearToPresent.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            barrierFromClearToPresent.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrierFromClearToPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            barrierFromClearToPresent.srcQueueFamilyIndex = 0;
            barrierFromClearToPresent.dstQueueFamilyIndex = 0;
            barrierFromClearToPresent.image = m_FrameBuffer->GetFrameImages()[imageIndex_].GetImage();
            barrierFromClearToPresent.subresourceRange = imageSubresource;

            vkCmdPipelineBarrier(m_CurrentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, 
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierFromClearToPresent);
        }

        virtual void EndFrame()
        {
            vkEndCommandBuffer(m_CurrentCommandBuffer);
        }

        virtual void RecreateResources()
        {

        }

    private:

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
            // m_FrameBuffer->CreateFrameBuffers(m_RenderPass, m_VulkanSwapchain->GetExtent());
            m_FrameBuffer->CreateCommandBuffers();
        }
    };
}

static VkInstance s_Instance{ VK_NULL_HANDLE };
static VkPhysicalDevice s_PhysicalDevice{ VK_NULL_HANDLE };
static VkDevice s_Device{ VK_NULL_HANDLE };
static VkQueue s_Queue{ VK_NULL_HANDLE };
static const uint32_t s_QueueIndex{ 0 };

static VkSurfaceKHR s_Surface{ VK_NULL_HANDLE };
static VkExtent2D s_Extent{};
static VkSurfaceFormatKHR s_SurfaceFormat{};
static VkPresentModeKHR s_PresentMode{};

// Viewport
static VkCommandPool s_ViewportCommandPool;
static std::vector<VkFramebuffer> s_ViewportFrameBuffers;
static std::vector<VkCommandBuffer> s_ViewportCommandBuffers;

uint32_t FindMemoryType(const uint32_t typeFilter_, const VkMemoryPropertyFlags flags_)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(s_PhysicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter_ & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & flags_) == flags_) {
            return i;
        }
    }

    return UINT32_MAX;
}

void BindBuffer(const CreateBufferSettings &bufferSettings_, 
        VkBuffer &buffer_, VkDeviceMemory &bufferMemory_) 
{
    VkBufferCreateInfo bufferCI{};
    bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCI.size = bufferSettings_.size;
    bufferCI.usage = bufferSettings_.usage;
    bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(s_Device, &bufferCI, nullptr, &buffer_);

    VkMemoryRequirements memRequirements{};
    vkGetBufferMemoryRequirements(s_Device, buffer_, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, bufferSettings_.properties);

    vkAllocateMemory(s_Device, &allocInfo, nullptr, &bufferMemory_);

    vkBindBufferMemory(s_Device, buffer_, bufferMemory_, 0);
}

VkCommandBuffer BeginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = s_ViewportCommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(s_Device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void EndSingleTimeCommands(VkCommandBuffer commandBuffer_) {
    vkEndCommandBuffer(commandBuffer_);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer_;

    // TODO: submit via transfer queue, need Transfer Command pool
    // https://vulkan-tutorial.com/Vertex_buffers/Staging_buffer

    vkQueueSubmit(s_Queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(s_Queue);

    vkFreeCommandBuffers(s_Device, s_ViewportCommandPool, 1, &commandBuffer_);
}

void CopyBuffer(VkBuffer srcBuffer_, VkBuffer dstBuffer_, VkDeviceSize size_)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
    {
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size_;

        vkCmdCopyBuffer(commandBuffer, srcBuffer_, dstBuffer_, 1, &copyRegion);
    }
    EndSingleTimeCommands(commandBuffer);
}

bool LoadModel(std::string &&path_) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path_.c_str())) {
        throw std::runtime_error(warn + err);
    }

    std::unordered_map<VertexData, uint32_t> uniqueVertices;
    uniqueVertices.reserve(attrib.vertices.size());

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            VertexData vertex{};

            vertex.position = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = {1.f, 1.f, 1.f};

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(s_Vertices.size());
                s_Vertices.emplace_back(vertex);
            }

            s_Indices.emplace_back(uniqueVertices[vertex]);
        }
    }

    return true;
}

bool CreateVertexBuffer() {
    VkDeviceSize bufferSize = static_cast<uint64_t>(sizeof(s_Vertices[0]) * s_Vertices.size());

    VkBuffer stagingBuffer{VK_NULL_HANDLE};
    VkDeviceMemory stagingBufferMemory{VK_NULL_HANDLE};
    CreateBufferSettings bufferSettings{};
    bufferSettings.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    bufferSettings.size = bufferSize;
    bufferSettings.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    BindBuffer(bufferSettings, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(s_Device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, s_Vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(s_Device, stagingBufferMemory);

    bufferSettings.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    bufferSettings.size = bufferSize;
    bufferSettings.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    BindBuffer(bufferSettings, s_VertexBuffer, s_VertexBufferMemory);
    
    CopyBuffer(stagingBuffer, s_VertexBuffer, bufferSize);

    vkDestroyBuffer(s_Device, stagingBuffer, nullptr);
    vkFreeMemory(s_Device, stagingBufferMemory, nullptr);
    return true;
}

bool CreateIndexBuffer() {
    VkDeviceSize bufferSize = static_cast<uint64_t>(sizeof(s_Indices[0]) * s_Indices.size());

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBufferSettings bufferSettings{};
    bufferSettings.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    bufferSettings.size = bufferSize;
    bufferSettings.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    BindBuffer(bufferSettings, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(s_Device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, s_Indices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(s_Device, stagingBufferMemory);

    bufferSettings.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    bufferSettings.size = bufferSize;
    bufferSettings.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    BindBuffer(bufferSettings, s_IndexBuffer, s_IndexBufferMemory);

    CopyBuffer(stagingBuffer, s_IndexBuffer, bufferSize);

    vkDestroyBuffer(s_Device, stagingBuffer, nullptr);
    vkFreeMemory(s_Device, stagingBufferMemory, nullptr);
    return true;
}

void TransitionImageLayout(VkImageLayout oldLayout_, VkImageLayout newLayout_, VkImage& image_)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout_;
        barrier.newLayout = newLayout_;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image_;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout_ == VK_IMAGE_LAYOUT_UNDEFINED && newLayout_ == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout_ == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout_ == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else if (oldLayout_ == VK_IMAGE_LAYOUT_UNDEFINED && newLayout_ == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else {
            throw std::invalid_argument("Unsupported layout transition!");
        }

        vkCmdPipelineBarrier( commandBuffer, sourceStage, destinationStage,
            0, 0, nullptr, 0, nullptr, 1, &barrier);
    }
    EndSingleTimeCommands(commandBuffer);
}

void CopyBufferToImage(VkBuffer stagingBuffer_, const VkImageCreateInfo& imageCI_) {
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
    {
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            imageCI_.extent.width,
            imageCI_.extent.height,
            1
        };

        vkCmdCopyBufferToImage(commandBuffer, stagingBuffer_, s_ObjectImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }
    EndSingleTimeCommands(commandBuffer);
}

void CreateImage(const VkImageCreateInfo &imageCI_, const VkMemoryPropertyFlags flags_, VkImage& image_, VkDeviceMemory& memory_) 
{
    vkCreateImage(s_Device, &imageCI_, nullptr, &image_);

    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(s_Device, image_, &memReq);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memReq.memoryTypeBits, flags_);

    vkAllocateMemory(s_Device, &allocInfo, nullptr, &memory_);

    vkBindImageMemory(s_Device, image_, memory_, 0);
}

void CreateImageView(VkFormat format_, VkImageAspectFlags aspect_, VkImage& image_, VkImageView& view_) 
{
    VkImageViewCreateInfo imageViewCI{};
    imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCI.subresourceRange.baseMipLevel = 0;
    imageViewCI.subresourceRange.levelCount = 1;
    imageViewCI.subresourceRange.baseArrayLayer = 0;
    imageViewCI.subresourceRange.layerCount = 1;
    imageViewCI.image = image_;
    imageViewCI.format = format_;
    imageViewCI.subresourceRange.aspectMask = aspect_;

    vkCreateImageView(s_Device, &imageViewCI, nullptr, &view_);
}

bool LoadTexture(std::string&& path_, VkImageCreateInfo& imageCI_) {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(path_.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    if (!pixels) {
        return false;
    }

    // m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    VkDeviceSize imageSize = static_cast<uint32_t>(texWidth) * static_cast<uint32_t>(texHeight) * 4;
    VkBuffer stagingBuffer{VK_NULL_HANDLE};
    VkDeviceMemory stagingBufferMemory{VK_NULL_HANDLE};
    CreateBufferSettings bufferSettings{};
    bufferSettings.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    bufferSettings.size = imageSize;
    bufferSettings.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    BindBuffer(bufferSettings, stagingBuffer, stagingBufferMemory);
        
    void* data;
    vkMapMemory(s_Device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(s_Device, stagingBufferMemory);

    stbi_image_free(pixels);

    imageCI_.extent.width = static_cast<uint32_t>(texWidth);
    imageCI_.extent.height = static_cast<uint32_t>(texHeight);

    CreateImage(imageCI_, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, s_ObjectImage, s_ObjectImageDeviceMemory);

    TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, s_ObjectImage);
    CopyBufferToImage(stagingBuffer, imageCI_);
    TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, s_ObjectImage);

    vkDestroyBuffer(s_Device, stagingBuffer, nullptr);
    vkFreeMemory(s_Device, stagingBufferMemory, nullptr);

    // GenerateMipmaps(VK_FORMAT_R8G8B8A8_UNORM);
    return true;
}

void CreateSampler() {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(s_PhysicalDevice, &properties);

    VkSamplerCreateInfo samplerCI{};
    samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCI.magFilter = VK_FILTER_LINEAR;
    samplerCI.minFilter = VK_FILTER_LINEAR;
    samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCI.anisotropyEnable = VK_TRUE;
    samplerCI.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerCI.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerCI.unnormalizedCoordinates = VK_FALSE;
    samplerCI.compareEnable = VK_FALSE;
    samplerCI.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCI.minLod = 0.f;
    // samplerCI.maxLod = static_cast<float>(m_MipLevels);
    samplerCI.maxLod = 1.f;
    samplerCI.mipLodBias = 0.f;

    vkCreateSampler(s_Device, &samplerCI, nullptr, &s_ObjectImageSampler);
}

void CreateUniformBuffers(uint32_t maxFrames_) {
    VkDeviceSize bufferSize = static_cast<uint64_t>(sizeof(UniformBufferObject));

    CreateBufferSettings bufferSettings{};
    bufferSettings.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    bufferSettings.size = bufferSize;
    bufferSettings.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    BindBuffer(bufferSettings, s_UniformBuffer, s_UniformBufferMemory);
    vkMapMemory(s_Device, s_UniformBufferMemory, 0, bufferSize, 0, &s_UniformBufferMapped);
}

bool CreateDescriptorPool(uint32_t maxFrames_) {
    std::array<VkDescriptorPoolSize, 2> poolSize{};
    poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize[0].descriptorCount = static_cast<uint32_t>(maxFrames_);
    poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize[1].descriptorCount = static_cast<uint32_t>(maxFrames_);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSize.size());
    poolInfo.pPoolSizes = poolSize.data();
    poolInfo.maxSets = static_cast<uint32_t>(maxFrames_);

    return vkCreateDescriptorPool(s_Device, &poolInfo, nullptr, &s_DescriptorPool) == VK_SUCCESS;
}

void CreateDescriptorSets(uint32_t maxFrames_, const VkSampler& sampler_, 
        const VkImageView& imageView_, VkDescriptorSetLayout layout_) {
    std::vector<VkDescriptorSetLayout> layouts(maxFrames_, layout_);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = s_DescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(maxFrames_);
    allocInfo.pSetLayouts = layouts.data();

    vkAllocateDescriptorSets(s_Device, &allocInfo, &s_DescriptorSet);

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = s_UniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = imageView_;
    imageInfo.sampler = sampler_;

    std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = s_DescriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;
    descriptorWrites[0].pImageInfo = nullptr;
    descriptorWrites[0].pTexelBufferView = nullptr;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = s_DescriptorSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(s_Device, static_cast<uint32_t>(descriptorWrites.size())
        , descriptorWrites.data(), 0, nullptr);
}

void UpdateUniformBuffer(uint32_t imageIndex_) 
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(10.0f), glm::vec3(0.f, 0.f, 1.f));
    ubo.view = glm::lookAt(glm::vec3(2.f, 2.f, 2.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 1.f));
    ubo.proj = glm::perspective(glm::radians(45.0f), s_Extent.width / static_cast<float>(s_Extent.height), 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;

    memcpy(s_UniformBufferMapped, &ubo, sizeof(ubo));
}

VkFormat FindSupportedFormat(const std::vector<VkFormat> &formats, 
    VkImageTiling tiling_, VkFormatFeatureFlags features_) 
{
    for (auto&& format : formats)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(s_PhysicalDevice, format, &props);

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

VkFormat FindDepthFormat() 
{
    return FindSupportedFormat(
				{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
				VK_IMAGE_TILING_OPTIMAL,
				VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void LoadObject()
{
    LoadModel("viking_room.obj");
    CreateVertexBuffer();
    CreateIndexBuffer();

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
    imageCI.queueFamilyIndexCount = 1;
    imageCI.pQueueFamilyIndices = &s_QueueIndex;
    imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    LoadTexture("viking_room.png", imageCI);
    CreateImageView(imageCI.format, VK_IMAGE_ASPECT_COLOR_BIT, s_ObjectImage, s_ObjectImageView);
    CreateSampler();

    CreateUniformBuffers(1);
    CreateDescriptorPool(1);
    // CreateDescriptorSets(1, s_ObjectImageSampler, s_ObjectImageView, s_ViewportDescriptorSetLayout);
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
        pipelines.emplace_back(new Victory::ImGuiPipeline());
        pipelines.emplace_back(new Victory::ViewportPipeline());

        for (auto&& pipeline : pipelines)
        {
            pipeline->InitResources(m_VulkanSwapchain->GetSurfaceFormat().format, 
                m_VulkanSwapchain->GetExtent(), m_VulkanSwapchain->GetImageCount());
        }

        // LoadObject();

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
                    continue;
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

            for (auto&& pipeline : pipelines)
            {
                delete pipeline;
            }

            CleanupSemaphores();
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
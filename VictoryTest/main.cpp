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

const static std::string ApplicationName{ "Testing features application" };
const static std::string EngineName{ "Testing Engine" };
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
        };

        VkDescriptorSetLayout GetDescriptorSetLayout() const
        {
            return m_DescriptorSetLayout;
        }

        VkBuffer GetUniformBuffer() const 
        {
            return m_UniformBuffer;
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
        Victory::ViewportPipeline* ViewportPipeline{ new Victory::ViewportPipeline() };
        pipelines.emplace_back(ViewportPipeline);

        for (auto&& pipeline : pipelines)
        {
            pipeline->InitResources(m_VulkanSwapchain->GetSurfaceFormat().format, 
                m_VulkanSwapchain->GetExtent(), m_VulkanSwapchain->GetImageCount());
        }

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

            models.clear();

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
#include "VulkanPipeline.h"

#include "../../Utils.h"
#include "VulkanVertexData.h"

bool VulkanPipeline::CreateRenderPass(VkPhysicalDevice phDevice_, VkDevice device_, VkFormat format_) {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = format_;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = FindDepthFormat(phDevice_);
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    // The index of the attachment in this array is directly referenced from the 
    // fragment shader with the layout(location = 0) out vec4 outColor directive!

    std::array<VkAttachmentDescription, 2> attachmets{colorAttachment, depthAttachment};

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassCI{};
    renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCI.attachmentCount = static_cast<uint32_t>(attachmets.size());
    renderPassCI.pAttachments = attachmets.data();
    renderPassCI.subpassCount = 1;
    renderPassCI.pSubpasses = &subpass;
    renderPassCI.dependencyCount = 1;
    renderPassCI.pDependencies = &dependency;

    return vkCreateRenderPass(device_, &renderPassCI, nullptr, &m_RenderPass) == VK_SUCCESS;
}

bool VulkanPipeline::CreateDescriptorSetLayout(VkDevice device_) {
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

    return (vkCreateDescriptorSetLayout(device_, &descriptorSetLayoutCI, nullptr, &m_DescriptorSetLayout) == VK_SUCCESS);
}

bool VulkanPipeline::CreatePipelineLayout(VkDevice device_) {
    VkPipelineLayoutCreateInfo pipelineLayoutCI{};
    pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCI.setLayoutCount = 1;
    pipelineLayoutCI.pSetLayouts = &m_DescriptorSetLayout;
    pipelineLayoutCI.pushConstantRangeCount = 0;
    pipelineLayoutCI.pPushConstantRanges = nullptr;

    return vkCreatePipelineLayout(device_, &pipelineLayoutCI, nullptr, &m_PipelineLayout) == VK_SUCCESS;
}

bool VulkanPipeline::CreatePipeline(VulkanContext &context_) {
    VkDevice device = context_.GetDevice();

    std::vector<char> vsBuffer = Utils::ReadFile("triangle.vert.spv");
    std::vector<char> fsBuffer = Utils::ReadFile("triangle.frag.spv");
    if (LoadShader(device, vsBuffer, &VS) != true) {
        printf("\nVertex shader was not loaded!");
        return false;
    }
    if (LoadShader(device, fsBuffer, &FS) != true) {
        printf("\nFragment shader was not loaded!");
        return false;
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

    // TODO: Rid of hard code data from vert shader
    auto&& bindingDescription = VulkanRenderer::GetBindingDescription();
    auto&& attributegDescriptions = VulkanRenderer::GetAttributeDescriptions();

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

    // Viewport the transformation from the image to the framebuffer
    // Scissor which regions pixels will actually be stored
    // https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions

    // VkViewport viewport{};
    // viewport.x = 0.0f;
    // viewport.y = 0.0f;
    // viewport.width = 1080.f;
    // viewport.height = 720.f;
    // viewport.minDepth = 0.0f;
    // viewport.maxDepth = 1.0f;

    // VkRect2D scissor{};
    // scissor.setOffset({0,0});
    // scissor.setExtent({1080,720});

    VkPipelineViewportStateCreateInfo viewportStateCI{};
    viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCI.viewportCount = 1;
    viewportStateCI.scissorCount = 1;
    // viewportStateCI.pViewports = &viewport;
    // viewportStateCI.pScissors = &scissor;

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
    multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleStateCI.minSampleShading = 1.f;
    multisampleStateCI.pSampleMask = nullptr;
    multisampleStateCI.alphaToCoverageEnable = VK_FALSE;
    multisampleStateCI.alphaToOneEnable = VK_FALSE;

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

    return vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineCI, nullptr, &m_Pipeline) == VK_SUCCESS;
}

void VulkanPipeline::CleanupPipeline(VkDevice device_) {
    vkDestroyPipeline(device_, m_Pipeline, nullptr);
}

void VulkanPipeline::CleanupPipelineLayout(VkDevice device_) {
    vkDestroyPipelineLayout(device_, m_PipelineLayout, nullptr);
}

void VulkanPipeline::CleanupDescriptorSet(VkDevice device_) {
    vkDestroyDescriptorSetLayout(device_, m_DescriptorSetLayout, nullptr);
}

void VulkanPipeline::CleanupRenderPass(VkDevice device_) {
    vkDestroyRenderPass(device_, m_RenderPass, nullptr);
}

void VulkanPipeline::CleanupShaderModule(VkDevice device_) {
    vkDestroyShaderModule(device_, FS, nullptr);
    vkDestroyShaderModule(device_, VS, nullptr);
}

void VulkanPipeline::CleanupAll(VulkanContext &context_) {
    VkDevice device = context_.GetDevice();
    CleanupPipeline(device);
    CleanupPipelineLayout(device);
    CleanupDescriptorSet(device);
    CleanupRenderPass(device);
    CleanupShaderModule(device);
}

VkRenderPass VulkanPipeline::GetRenderPass() const {
    return m_RenderPass;
}

VkPipelineLayout VulkanPipeline::GetPipelineLayout() const {
    return m_PipelineLayout;
}

VkPipeline VulkanPipeline::GetPipeline() const {
    return m_Pipeline;
}

bool VulkanPipeline::LoadShader(VkDevice device_, const std::vector<char>&buffer_, VkShaderModule* shaderModule_) {
    if (!shaderModule_) {
        return false;
    }

    VkShaderModuleCreateInfo shaderModuleCI{};
    shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCI.codeSize = static_cast<uint32_t>(buffer_.size());
    shaderModuleCI.pCode = reinterpret_cast<const uint32_t*>(buffer_.data());

    return vkCreateShaderModule(device_, &shaderModuleCI, nullptr, shaderModule_) == VK_SUCCESS;
}

VkFormat VulkanPipeline::FindSupportedFormat(VkPhysicalDevice phDevice_, const std::vector<VkFormat>& candidates_, VkImageTiling tiling_, VkFormatFeatureFlags features_) {
    for (auto&& format : candidates_) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(phDevice_, format, &props);

        if (tiling_ == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features_) == features_) {
            return format;
        }

        if (tiling_ == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features_) == features_) {
            return format;
        }
    }

    throw std::runtime_error("Filed to find supported formt!");
}

VkFormat VulkanPipeline::FindDepthFormat(VkPhysicalDevice phDevice_) {
    return FindSupportedFormat(phDevice_,
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}
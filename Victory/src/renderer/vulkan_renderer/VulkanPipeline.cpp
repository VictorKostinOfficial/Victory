#include "VulkanPipeline.h"
#include "VulkanContext.h"
#include "VulkanSwapchain.h"

#include <stdio.h>

bool VulkanPipeline::CreatePipeline(VulkanContext* context_, VulkanSwapchain* swapchain_) {
    vk::Device device = context_->GetDevice();

    // TODO: make function
    vk::ShaderModule VS = 
        VulkanPipeline::LoadShader(device, "shaders/triangle.vert.spv");
    if (!VS) {
        return false;
    }
    m_ShaderModules.emplace_back(VS);
    vk::ShaderModule FS = 
        VulkanPipeline::LoadShader(device, "shaders/triangle.frag.spv");
    if (!FS) {
        return false;
    }
    m_ShaderModules.emplace_back(FS);

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStageCIs;
    shaderStageCIs.reserve(m_ShaderModules.size());
    for (auto&& shaderModule : m_ShaderModules) {
        vk::PipelineShaderStageCreateInfo shaderStageCI{
        {},
        vk::ShaderStageFlagBits::eVertex,
        shaderModule,
        "main"
        };

        shaderStageCIs.emplace_back(shaderStageCI);
    }

    std::vector<vk::DynamicState> dynamicStates{
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamicStateCI{};
    dynamicStateCI.setDynamicStates(dynamicStates);

    // TODO: Rid of hard code data from vert shader
    vk::PipelineVertexInputStateCreateInfo vertexInputStateCI{};

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{};
    inputAssemblyStateCI.setTopology(vk::PrimitiveTopology::eTriangleList);
    inputAssemblyStateCI.setPrimitiveRestartEnable(VK_FALSE);

    // Viewport the transformation from the image to the framebuffer
    // Scissor which regions pixels will actually be stored
    // https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions

    vk::Extent2D extent = swapchain_->GetExtent();
    vk::Viewport viewport{};
    viewport.setX(0.f);
    viewport.setY(0.f);
    viewport.setWidth(static_cast<float>(extent.width));
    viewport.setWidth(static_cast<float>(extent.height));
    viewport.setMinDepth(0.f);
    viewport.setMaxDepth(1.f);

    vk::Rect2D scissor{};
    scissor.setOffset({0,0});
    scissor.setExtent(extent);

    // Static state?
    vk::PipelineViewportStateCreateInfo viewportStateCI{};
    viewportStateCI.setViewports(viewport);
    viewportStateCI.setScissors(scissor);

    vk::PipelineRasterizationStateCreateInfo rasterizationStateCI{};
    rasterizationStateCI.setDepthClampEnable(VK_FALSE);
    rasterizationStateCI.setRasterizerDiscardEnable(VK_FALSE);
    rasterizationStateCI.setPolygonMode(vk::PolygonMode::eFill);
    rasterizationStateCI.setLineWidth(1.f);
    rasterizationStateCI.setCullMode(vk::CullModeFlagBits::eBack);
    rasterizationStateCI.setFrontFace(vk::FrontFace::eClockwise);
    rasterizationStateCI.setDepthBiasEnable(VK_FALSE);
    rasterizationStateCI.setDepthBiasConstantFactor(0.f);
    rasterizationStateCI.setDepthBiasClamp(0.f);
    rasterizationStateCI.setDepthBiasSlopeFactor(0.f);

    vk::PipelineMultisampleStateCreateInfo MultisampleStateCI{};
    MultisampleStateCI.setSampleShadingEnable(VK_FALSE);
    MultisampleStateCI.setRasterizationSamples(vk::SampleCountFlagBits::e1);
    MultisampleStateCI.setMinSampleShading(1.f);
    MultisampleStateCI.setPSampleMask(nullptr);
    MultisampleStateCI.setAlphaToCoverageEnable(VK_FALSE);
    MultisampleStateCI.setAlphaToOneEnable(VK_FALSE);

    vk::PipelineDepthStencilStateCreateInfo depthStencilStateCI{};

    vk::PipelineColorBlendAttachmentState colorBlendAttachmentState{};
    colorBlendAttachmentState.setColorWriteMask(vk::ColorComponentFlagBits::eR | 
                                                vk::ColorComponentFlagBits::eG | 
                                                vk::ColorComponentFlagBits::eB | 
                                                vk::ColorComponentFlagBits::eA);
    colorBlendAttachmentState.setBlendEnable(VK_FALSE);
    colorBlendAttachmentState.setSrcColorBlendFactor(vk::BlendFactor::eOne);
    colorBlendAttachmentState.setDstColorBlendFactor(vk::BlendFactor::eZero);
    colorBlendAttachmentState.setColorBlendOp(vk::BlendOp::eAdd);
    colorBlendAttachmentState.setSrcAlphaBlendFactor(vk::BlendFactor::eOne);
    colorBlendAttachmentState.setDstAlphaBlendFactor(vk::BlendFactor::eZero);
    colorBlendAttachmentState.setAlphaBlendOp(vk::BlendOp::eAdd);

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCI{};
    colorBlendStateCI.setLogicOpEnable(VK_FALSE);
    colorBlendStateCI.setLogicOp(vk::LogicOp::eCopy);
    colorBlendStateCI.setAttachments(colorBlendAttachmentState);
    colorBlendStateCI.setBlendConstants({0.f, 0.f, 0.f, 0.f});

    vk::PipelineLayoutCreateInfo pipelineLayoutCI{};
    pipelineLayoutCI.setSetLayoutCount(0);
    pipelineLayoutCI.setPSetLayouts(nullptr);
    pipelineLayoutCI.setPushConstantRangeCount(0);
    pipelineLayoutCI.setPPushConstantRanges(nullptr);

    m_PipelineLayout = device.createPipelineLayout(pipelineLayoutCI);


    // ------------------------------
    // TODO: create function for creating Render pass
    // ------------------------------

    vk::AttachmentDescription colorAttachment{};
    colorAttachment.setFormat(swapchain_->GetSwapcahinFormat());
    colorAttachment.setSamples(vk::SampleCountFlagBits::e1);
    colorAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
    colorAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
    colorAttachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
    colorAttachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
    colorAttachment.setInitialLayout(vk::ImageLayout::eUndefined);
    colorAttachment.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentReference colorAttachmentRef{};
    colorAttachmentRef.setAttachment(0);
    colorAttachmentRef.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    vk::SubpassDescription subpass{};
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
    subpass.setColorAttachments(colorAttachmentRef);
    // The index of the attachment in this array is directly referenced from the 
    // fragment shader with the layout(location = 0) out vec4 outColor directive!

    vk::RenderPassCreateInfo renderPassCI{};
    renderPassCI.setAttachments(colorAttachment);
    renderPassCI.setSubpasses(subpass);

    m_RenderPass = device.createRenderPass(renderPassCI);

    // ------------------------------
    // TODO: create function for creating Pipeline
    // ------------------------------

    vk::GraphicsPipelineCreateInfo pipelineCI{};
    pipelineCI.setStages(shaderStageCIs);
    pipelineCI.setPVertexInputState(&vertexInputStateCI);
    pipelineCI.setPInputAssemblyState(&inputAssemblyStateCI);
    pipelineCI.setPViewportState(&viewportStateCI);
    pipelineCI.setPRasterizationState(&rasterizationStateCI);
    pipelineCI.setPMultisampleState(&MultisampleStateCI);
    pipelineCI.setPDepthStencilState(&depthStencilStateCI);
    pipelineCI.setPColorBlendState(&colorBlendStateCI);
    pipelineCI.setPDynamicState(&dynamicStateCI);
    pipelineCI.setLayout(m_PipelineLayout);
    pipelineCI.setRenderPass(m_RenderPass);
    pipelineCI.setSubpass(0); // Index of render pass subpass
    pipelineCI.setBasePipelineHandle(VK_NULL_HANDLE);
    pipelineCI.setBasePipelineIndex(-1);

    m_Pipeline = device.createGraphicsPipeline(VK_NULL_HANDLE, pipelineCI).value;
    return false;
}

bool VulkanPipeline::CreatePipelineLayout() {
    return false;
}

void VulkanPipeline::Cleanup(VulkanContext* context_) {
    vk::Device device = context_->GetDevice();

    device.destroyPipeline(m_Pipeline);
    device.destroyRenderPass(m_RenderPass);
    device.destroyPipelineLayout(m_PipelineLayout);
    for (auto&& shaderModule : m_ShaderModules) {
        device.destroyShaderModule(shaderModule);
    }
}

vk::ShaderModule VulkanPipeline::LoadShader(vk::Device device_, std::string&& path_) {
    // TODO: create utils for read file?
    FILE* file = fopen(path_.c_str(), "rb");
    if (file) {
        // TODO: Handle error
        printf("\nFile nor readed: %s", path_.c_str());
        return VK_NULL_HANDLE;
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

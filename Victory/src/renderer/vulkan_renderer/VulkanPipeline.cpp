#include "VulkanPipeline.h"
#include "VulkanContext.h"
#include "VulkanSwapchain.h"

#include "../../Utils.h"

bool VulkanPipeline::CreatePipeline(VulkanContext* context_) {
    vk::Device device = context_->GetDevice();

    std::vector<char> vsBuffer = Utils::ReadFile("../../Victory/shaders/triangle.vert.spv");
    std::vector<char> fsBuffer = Utils::ReadFile("../../Victory/shaders/triangle.frag.spv");
    VS = LoadShader(device, vsBuffer);
    FS = LoadShader(device, fsBuffer);

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStageCIs{
        {
            {},
            vk::ShaderStageFlagBits::eVertex,
            VS,
            "main"
        },
        {
            {},
            vk::ShaderStageFlagBits::eFragment,
            FS,
            "main"
        }
    };

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

    vk::PipelineTessellationStateCreateInfo tessellationStateCI{};

    // Viewport the transformation from the image to the framebuffer
    // Scissor which regions pixels will actually be stored
    // https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions

    vk::PipelineViewportStateCreateInfo viewportStateCI{};
    viewportStateCI.setViewportCount(1);
    viewportStateCI.setScissorCount(1);

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

    vk::GraphicsPipelineCreateInfo pipelineCI{};
    pipelineCI.setStages(shaderStageCIs);
    pipelineCI.setPVertexInputState(&vertexInputStateCI);
    pipelineCI.setPInputAssemblyState(&inputAssemblyStateCI);
    pipelineCI.setPTessellationState(&tessellationStateCI);
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
    return true;
}

bool VulkanPipeline::CreateRenderPass(vk::Device device_, vk::Format format_) {
    vk::AttachmentDescription colorAttachment{};
    colorAttachment.setFormat(format_);
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

    vk::SubpassDependency dependecy{};
    dependecy.setSrcSubpass(vk::SubpassExternal);
    dependecy.setDstSubpass(0);
    dependecy.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    dependecy.setSrcAccessMask(vk::AccessFlagBits::eNone);
    dependecy.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    dependecy.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

    vk::RenderPassCreateInfo renderPassCI{};
    renderPassCI.setAttachments(colorAttachment);
    renderPassCI.setSubpasses(subpass);
    renderPassCI.setDependencies(dependecy);

    m_RenderPass = device_.createRenderPass(renderPassCI);
    return true;
}

bool VulkanPipeline::CreatePipelineLayout(vk::Device device_)
{
    vk::PipelineLayoutCreateInfo pipelineLayoutCI{};
    pipelineLayoutCI.setSetLayoutCount(0);
    pipelineLayoutCI.setPSetLayouts(nullptr);
    pipelineLayoutCI.setPushConstantRangeCount(0);
    pipelineLayoutCI.setPPushConstantRanges(nullptr);

    m_PipelineLayout = device_.createPipelineLayout(pipelineLayoutCI);
    return true;
}

void VulkanPipeline::Cleanup(VulkanContext* context_) {
    vk::Device device = context_->GetDevice();

    device.destroyPipeline(m_Pipeline);
    device.destroyPipelineLayout(m_PipelineLayout);
    device.destroyRenderPass(m_RenderPass);
    device.destroyShaderModule(FS);
    device.destroyShaderModule(VS);
}

const vk::RenderPass VulkanPipeline::GetRenderPass() const {
    return m_RenderPass;
}

const vk::Pipeline VulkanPipeline::GetPipeline() const {
    return m_Pipeline;
}

vk::ShaderModule VulkanPipeline::LoadShader(vk::Device device_, std::vector<char> buffer_) {
    vk::ShaderModuleCreateInfo shaderModuleCI{};
    shaderModuleCI.setCodeSize(buffer_.size());
    shaderModuleCI.setPCode(reinterpret_cast<const uint32_t*>(buffer_.data()));

    return device_.createShaderModule(shaderModuleCI);
}

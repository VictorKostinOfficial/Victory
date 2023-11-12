#pragma once

#include <vulkan/vulkan.hpp>

class VulkanContext;
class VulkanSwapchain;

class VulkanPipeline {
public:

    bool CreatePipeline(VulkanContext* context_, VulkanSwapchain* swapchain_);
    bool CreatePipelineLayout();
    void Cleanup(VulkanContext* context_);

    static vk::ShaderModule LoadShader(vk::Device device_, std::string&& path_);

private:

    std::vector<vk::ShaderModule> m_ShaderModules;

    vk::Pipeline m_Pipeline{VK_NULL_HANDLE};
    vk::PipelineLayout m_PipelineLayout{VK_NULL_HANDLE};
    vk::RenderPass m_RenderPass{VK_NULL_HANDLE};
};
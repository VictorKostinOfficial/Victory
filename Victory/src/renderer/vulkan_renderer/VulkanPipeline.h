#pragma once

#include <vulkan/vulkan.hpp>

class VulkanContext;
class VulkanSwapchain;

class VulkanPipeline {
public:

    bool CreateRenderPass(vk::Device device_, vk::Format format_);
    bool CreatePipelineLayout(vk::Device device_);
    bool CreatePipeline(VulkanContext* context_, VulkanSwapchain* swapchain_);
    void Cleanup(VulkanContext* context_);

    const vk::RenderPass GetRenderPass() const;
    const vk::Pipeline GetPipeline() const;

    vk::ShaderModule LoadShader(vk::Device device_, std::vector<char> buffer_);

private:

    vk::Pipeline m_Pipeline{VK_NULL_HANDLE};
    vk::PipelineLayout m_PipelineLayout{VK_NULL_HANDLE};
    vk::RenderPass m_RenderPass{VK_NULL_HANDLE};

    vk::ShaderModule VS{VK_NULL_HANDLE};
    vk::ShaderModule FS{VK_NULL_HANDLE};
};
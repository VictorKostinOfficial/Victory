#pragma once

#include <vulkan/vulkan_core.h>

#include "VulkanContext.h"
#include "VulkanSwapchain.h"

class VulkanBuffer;

class VulkanPipeline {
public:

    bool CreateRenderPass(VkPhysicalDevice phDevice, VkDevice device_, VkFormat format_);
    bool CreateDescriptorSetLayout(VkDevice device_);
    bool CreatePipelineLayout(VkDevice device_);
    bool CreatePipeline(VulkanContext& context_);

    VkFormat FindSupportedFormat(VkPhysicalDevice phDevice_, const std::vector<VkFormat>& candidates_, 
        VkImageTiling tiling_, VkFormatFeatureFlags features_);
    VkFormat FindDepthFormat(VkPhysicalDevice phDevice_);

    void CleanupPipeline(VkDevice device_);
    void CleanupPipelineLayout(VkDevice device_);
    void CleanupDescriptorSet(VkDevice device_);
    void CleanupRenderPass(VkDevice device_);
    void CleanupShaderModule(VkDevice device_);
    void CleanupAll(VulkanContext& context_);

    VkRenderPass GetRenderPass() const;
    VkPipelineLayout GetPipelineLayout() const;
    VkPipeline GetPipeline() const;
    inline VkDescriptorSetLayout GetDescriptorSetLayout() const {
        return m_DescriptorSetLayout;
    }

    bool LoadShader(VkDevice device_, const std::vector<char>& buffer_, VkShaderModule* shaderModule_);

private:

    VkPipeline m_Pipeline{VK_NULL_HANDLE};
    VkDescriptorSetLayout m_DescriptorSetLayout{VK_NULL_HANDLE};
    VkPipelineLayout m_PipelineLayout{VK_NULL_HANDLE};
    VkRenderPass m_RenderPass{VK_NULL_HANDLE};

    VkShaderModule VS{VK_NULL_HANDLE};
    VkShaderModule FS{VK_NULL_HANDLE};
};

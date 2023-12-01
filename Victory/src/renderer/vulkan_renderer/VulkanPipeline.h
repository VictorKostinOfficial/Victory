#pragma once

class VulkanContext;
class VulkanBuffer;

class VulkanPipeline {
public:

    VulkanPipeline(VulkanContext* context_);

    bool CreateRenderPass(VkFormat format_);
    bool CreatePipelineLayout();
    bool CreatePipeline();

    bool CreateDescriptorSetLayout();

    VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates_, 
        VkImageTiling tiling_, VkFormatFeatureFlags features_);
    VkFormat FindDepthFormat();

    void CleanupPipeline();
    void CleanupPipelineLayout();
    void CleanupDescriptorSet();
    void CleanupRenderPass();
    void CleanupShaderModule();
    void CleanupAll();

    VkRenderPass GetRenderPass() const;

    VkPipelineLayout GetPipelineLayout() const;

    VkPipeline GetPipeline() const;

    inline VkDescriptorSetLayout GetDescriptorSetLayout() const {
        return m_DescriptorSetLayout;
    }

    bool LoadShader(const std::vector<char>& buffer_, VkShaderModule* shaderModule_);

private:

    VulkanContext* m_Context;

    VkPipeline m_Pipeline{VK_NULL_HANDLE};
    VkPipelineLayout m_PipelineLayout{VK_NULL_HANDLE};
    VkRenderPass m_RenderPass{VK_NULL_HANDLE};

    VkDescriptorSetLayout m_DescriptorSetLayout{VK_NULL_HANDLE};

    VkShaderModule VS{VK_NULL_HANDLE};
    VkShaderModule FS{VK_NULL_HANDLE};
};

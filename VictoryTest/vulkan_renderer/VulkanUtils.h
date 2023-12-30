#pragma once

#include <iostream>
#include <string>

namespace Victory 
{
    inline void CheckVulkanResult(const VkResult result_, const std::string&& message_) 
    {
        if (result_ != VK_SUCCESS) 
        {
            std::cout << "ERROR: " << message_ << std::endl;
            throw std::runtime_error(message_);
        }
    }

    static void CreateShaderModule(VkDevice device, 
        const std::vector<char>& buffer_, VkShaderModule* shaderModule_) 
    {
        if (!shaderModule_)
        {
            return;
        }

        VkShaderModuleCreateInfo shaderModuleCI{};
        shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCI.codeSize = static_cast<uint32_t>(buffer_.size());
        shaderModuleCI.pCode = reinterpret_cast<const uint32_t*>(buffer_.data());

        CheckVulkanResult( 
            vkCreateShaderModule(device, 
                &shaderModuleCI, nullptr, shaderModule_),
            "Shader was not loaded");
    }
}
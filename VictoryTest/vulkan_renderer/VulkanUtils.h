#pragma once

#include <vulkan/vulkan.h>
#include <iostream>
#include <array>

#include "VertexData.h"

namespace Victory 
{
    static VkVertexInputBindingDescription GetBindingDescription() 
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(VertexData);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions() 
    {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = 0;

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = sizeof(float) * 3;

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = sizeof(float) * 6;

        return attributeDescriptions;
    }

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
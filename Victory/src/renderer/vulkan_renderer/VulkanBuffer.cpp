#include <vulkan/vulkan.h>
#include <vector>

#include "VulkanBuffer.h"
#include "../VertexData.h"

#include <cstring>
#include <array>
#include <unordered_map>
#include <stdexcept>
#include <chrono>

#include "VulkanContext.h"
#include "VulkanUtils.h"
#include "VulkanSwapchain.h"
#include "VulkanPipeline.h"
#include "VulkanFrameBuffer.h"

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "VulkanModel.h"

const std::string MODEL_PATH = "viking_room.obj";
const std::string TEXTURE_PATH = "viking_room.png";

std::vector<VertexData> vertices;

std::vector<uint16_t> indices;

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

VulkanBuffer::VulkanBuffer(VulkanContext *context_, VulkanPipeline* pipeline_, VulkanFrameBuffer *frameBuffer_, VulkanSwapchain* swapchain_) 
    : m_Context{context_}
    , m_Pipeline{pipeline_}
    , m_FrameBuffer{frameBuffer_}
    , m_Swapchain{swapchain_} {
}

bool VulkanBuffer::CreateUniformBuffers(uint32_t maxFrames_) {
    VkDeviceSize bufferSize = static_cast<uint64_t>(sizeof(UniformBufferObject));

    CreateBufferSettings bufferSettings{};
    bufferSettings.Properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    bufferSettings.Size = bufferSize;
    bufferSettings.Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    m_UniformBuffers.resize(maxFrames_);
    m_UniformBuffersMemory.resize(maxFrames_);
    m_UniformBuffersMapped.resize(maxFrames_);

    for (size_t i{0}; i < maxFrames_; ++i) {
        BindBuffer(m_Context, bufferSettings, m_UniformBuffers[i], m_UniformBuffersMemory[i]);
        vkMapMemory(m_Context->GetDevice(), m_UniformBuffersMemory[i], 0, bufferSize, 0, &m_UniformBuffersMapped[i]);
    }
    return true;
}

bool VulkanBuffer::CreateDescriptorPool(uint32_t maxFrames_) {
    std::array<VkDescriptorPoolSize, 2> poolSize{};
    poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize[0].descriptorCount = static_cast<uint32_t>(maxFrames_);
    poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize[1].descriptorCount = static_cast<uint32_t>(maxFrames_);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSize.size());
    poolInfo.pPoolSizes = poolSize.data();
    poolInfo.maxSets = static_cast<uint32_t>(maxFrames_);

    return vkCreateDescriptorPool(m_Context->GetDevice(), &poolInfo, nullptr, &m_DescriptorPool) == VK_SUCCESS;
}

bool VulkanBuffer::CreateDescriptorSets(uint32_t maxFrames_, const VkSampler& sampler_, const VkImageView& imageView_) {
    std::vector<VkDescriptorSetLayout> layouts(maxFrames_, m_Pipeline->GetDescriptorSetLayout());
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_DescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(maxFrames_);
    allocInfo.pSetLayouts = layouts.data();

    m_DescriptorSets.resize(maxFrames_);
    if (vkAllocateDescriptorSets(m_Context->GetDevice(), &allocInfo, m_DescriptorSets.data()) != VK_SUCCESS) {
        return false;
    }

    for (size_t i = 0; i < maxFrames_; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_UniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = imageView_;
        imageInfo.sampler = sampler_;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = m_DescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;
        descriptorWrites[0].pImageInfo = nullptr;
        descriptorWrites[0].pTexelBufferView = nullptr;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = m_DescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(m_Context->GetDevice(), static_cast<uint32_t>(descriptorWrites.size())
            , descriptorWrites.data(), 0, nullptr);
    }
    return true;
}

void VulkanBuffer::UpdateUniformBuffer(uint32_t imageIndex_) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.f, 0.f, 1.f));
    ubo.view = glm::lookAt(glm::vec3(2.f, 2.f, 2.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 1.f));
    ubo.proj = glm::perspective(glm::radians(45.0f), m_Swapchain->GetExtent().width / (float) m_Swapchain->GetExtent().height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;

    memcpy(m_UniformBuffersMapped[imageIndex_], &ubo, sizeof(ubo));
}

void VulkanBuffer::FreeMemory()
{
    for (auto&& memory : m_UniformBuffersMemory) {
        vkFreeMemory(m_Context->GetDevice(), memory, nullptr);
    }
}

void VulkanBuffer::CleanupVertexBuffer() {
    for (auto&& buffer : m_UniformBuffers) {
        vkDestroyBuffer(m_Context->GetDevice(), buffer, nullptr);
    }
}

void VulkanBuffer::CleanupAll() {
    vkDestroyDescriptorPool(m_Context->GetDevice(), m_DescriptorPool, nullptr);
    CleanupVertexBuffer();
    FreeMemory();
}

bool VulkanBuffer::HasStencilComponent(VkFormat format_) {
    return format_ == VK_FORMAT_D32_SFLOAT_S8_UINT || format_ == VK_FORMAT_D24_UNORM_S8_UINT;
}
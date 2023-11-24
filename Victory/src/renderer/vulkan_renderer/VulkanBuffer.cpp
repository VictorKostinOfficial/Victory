#include "VulkanBuffer.h"
#include <vector>
#include <cstring>
#include <chrono>
#include "../VertexData.h"

#include "VulkanFrameBuffer.h"
#include "VulkanPipeline.h"

#include <chrono>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

const std::vector<VertexData> vertices = {
    {{-0.5f, -0.5f, 0}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f, 0}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f, 0}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f, 0}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
};

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

bool VulkanBuffer::CreateVertexBuffer()
{
    VkDeviceSize bufferSize = static_cast<uint64_t>(sizeof(vertices[0]) * vertices.size());

    VkBuffer stagingBuffer{VK_NULL_HANDLE};
    VkDeviceMemory stagingBufferMemory{VK_NULL_HANDLE};

    if (CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory) != true) {
        return false;
    }

    void* data;
    vkMapMemory(m_Context->GetDevice(), stagingBufferMemory, 0, sizeof(vertices[0]) * vertices.size(), 0, &data);
        memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(m_Context->GetDevice(), stagingBufferMemory);

    if (CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VertexBuffer, m_VertexBufferMemory) != true) {
            return false;
    };
    
    CopyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);

    vkDestroyBuffer(m_Context->GetDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_Context->GetDevice(), stagingBufferMemory, nullptr);
    return true;
}

bool VulkanBuffer::CreateIndexBuffer() {
    VkDeviceSize bufferSize = static_cast<uint64_t>(sizeof(indices[0]) * indices.size());

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(m_Context->GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(m_Context->GetDevice(), stagingBufferMemory);

    CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_IndexBuffer, m_IndexBufferMemory);

    CopyBuffer(stagingBuffer, m_IndexBuffer, bufferSize);

    vkDestroyBuffer(m_Context->GetDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_Context->GetDevice(), stagingBufferMemory, nullptr);
    return true;
}

bool VulkanBuffer::CreateUniformBuffers(uint32_t maxFrames_) {
    VkDeviceSize bufferSize = static_cast<uint64_t>(sizeof(UniformBufferObject));

    m_UniformBuffers.resize(maxFrames_);
    m_UniformBuffersMemory.resize(maxFrames_);
    m_UniformBuffersMapped.resize(maxFrames_);

    for (size_t i{0}; i < maxFrames_; ++i) {
        CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
            m_UniformBuffers[i], m_UniformBuffersMemory[i]);

        vkMapMemory(m_Context->GetDevice(), m_UniformBuffersMemory[i], 0, bufferSize, 0, &m_UniformBuffersMapped[i]);
    }
    return true;
}

bool VulkanBuffer::CreateDescriptorPool(uint32_t maxFrames_) {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(maxFrames_);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = static_cast<uint32_t>(maxFrames_);

    return vkCreateDescriptorPool(m_Context->GetDevice(), &poolInfo, nullptr, &m_DescriptorPool) == VK_SUCCESS;
}

bool VulkanBuffer::CreateDescriptorSets(uint32_t maxFrames_) {
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

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_DescriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;
        descriptorWrite.pImageInfo = nullptr;
        descriptorWrite.pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(m_Context->GetDevice(), 1, &descriptorWrite, 0, nullptr);
    }
    return true;
}

bool VulkanBuffer::CreateBuffer(VkDeviceSize size_, VkBufferUsageFlags usage_, VkMemoryPropertyFlags properties_, VkBuffer& buffer_, VkDeviceMemory& bufferMemory_) {
    VkBufferCreateInfo bufferCI{};
    bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCI.size = size_;
    bufferCI.usage = usage_;
    bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_Context->GetDevice(), &bufferCI, nullptr, &buffer_) != VK_SUCCESS) {
        return false;
    }

    VkMemoryRequirements memRequirements{};
    vkGetBufferMemoryRequirements(m_Context->GetDevice(), buffer_, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties_);
    if (allocInfo.memoryTypeIndex == UINT32_MAX) {
        return false;
    }

    if (vkAllocateMemory(m_Context->GetDevice(), &allocInfo, nullptr, &bufferMemory_) != VK_SUCCESS) {
        return false;
    }

    return vkBindBufferMemory(m_Context->GetDevice(), buffer_, bufferMemory_, 0) == VK_SUCCESS;
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

void VulkanBuffer::FreeMemory() {
    size_t test = m_UniformBuffersMemory.size();
    test++;
    for (auto&& memory : m_UniformBuffersMemory) {
        vkFreeMemory(m_Context->GetDevice(), memory, nullptr);
    }
    vkFreeMemory(m_Context->GetDevice(), m_IndexBufferMemory, nullptr);
    vkFreeMemory(m_Context->GetDevice(), m_VertexBufferMemory, nullptr);
}

void VulkanBuffer::CleanupVertexBuffer() {
    for (auto&& buffer : m_UniformBuffers) {
        vkDestroyBuffer(m_Context->GetDevice(), buffer, nullptr);
    }
    vkDestroyBuffer(m_Context->GetDevice(), m_IndexBuffer, nullptr);
    vkDestroyBuffer(m_Context->GetDevice(), m_VertexBuffer, nullptr);
}

void VulkanBuffer::CleanupAll() {
    vkDestroyDescriptorPool(m_Context->GetDevice(), m_DescriptorPool, nullptr);
    CleanupVertexBuffer();
    FreeMemory();
}

uint32_t VulkanBuffer::GetIndicesCount() const {
    return static_cast<uint32_t>(indices.size());
}

uint32_t VulkanBuffer::FindMemoryType(uint32_t typeFilter_, VkMemoryPropertyFlags flags_) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_Context->GetPhysicalDevice(), &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter_ & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & flags_) == flags_) {
            return i;
        }
    }

    return UINT32_MAX;
}

void VulkanBuffer::CopyBuffer(VkBuffer srcBuffer_, VkBuffer dstBuffer_, VkDeviceSize size_) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_FrameBuffer->GetCommandPool();
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_Context->GetDevice(), &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    {
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size_;

        vkCmdCopyBuffer(commandBuffer, srcBuffer_, dstBuffer_, 1, &copyRegion);
    }
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    // TODO: submit via transfer queue, need Transfer Command pool
    // https://vulkan-tutorial.com/Vertex_buffers/Staging_buffer

    VkQueue queue = m_Context->GetQueue(QueueIndex::eGraphics);
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(m_Context->GetDevice(), m_FrameBuffer->GetCommandPool(), 1, &commandBuffer);
}

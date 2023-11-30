#include <vulkan/vulkan.h>
#include <vector>

#include "VulkanBuffer.h"
#include "../VertexData.h"

#include <cstring>
#include <array>
#include <stdexcept>
#include <chrono>

#include "VulkanContext.h"
#include "VulkanUtils.h"
#include "VulkanSwapchain.h"
#include "VulkanPipeline.h"
#include "VulkanFrameBuffer.h"

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

const std::string MODEL_PATH = "viking_room.obj";
const std::string TEXTURE_PATH = "viking_room.png";

std::vector<VertexData> vertices = {
    // {{-0.5f, -0.5f, 0}, {1.0f, 0.0f, 0.0f}, {1.f, 0.f}},
    // {{0.5f, -0.5f, 0}, {0.0f, 1.0f, 0.0f}, {0.f, 0.f}},
    // {{0.5f, 0.5f, 0}, {0.0f, 0.0f, 1.0f}, {0.f, 1.f}},
    // {{-0.5f, 0.5f, 0}, {1.0f, 1.0f, 1.0f}, {1.f, 1.f}},

    // {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    // {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    // {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    // {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};

std::vector<uint16_t> indices = {
    // 0, 1, 2, 2, 3, 0,
    // 4, 5, 6, 6, 7, 4
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

bool VulkanBuffer::LoadModel() {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
        throw std::runtime_error(warn + err);
    }

    indices.reserve(attrib.vertices.size());
    vertices.reserve(attrib.vertices.size());

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            VertexData vertex{};

            vertex.position = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = {1.f, 1.f, 1.f};

            vertices.emplace_back(vertex);
            indices.emplace_back(indices.size());
        }
    }
    return true;
}

// TODO: Move to VulkanFrameBuffer
bool VulkanBuffer::CreateDepthResources() {
    CreateImageSettings settings{};
    settings.Width = m_Swapchain->GetExtent().width;
    settings.Height = m_Swapchain->GetExtent().height;
    settings.Format = FindDepthFormat();
    settings.Tiling = VK_IMAGE_TILING_OPTIMAL;
    settings.Usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    settings.Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    CreateImage(settings, m_DepthImage, m_DepthImageMemory);
    
    m_Swapchain->CreateImageView(m_DepthImage, settings.Format, VK_IMAGE_ASPECT_DEPTH_BIT, m_DepthImageView);
    return true;
}

bool VulkanBuffer::CreateTextureImage() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    if (!pixels) {
        return false;
    }

    VkDeviceSize imageSize = static_cast<uint32_t>(texWidth * texHeight * 4);

    VkBuffer stagingBuffer{VK_NULL_HANDLE};
    VkDeviceMemory stagingBufferMemory{VK_NULL_HANDLE};
    CreateBufferSettings bufferSettings{};
    bufferSettings.Properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    bufferSettings.Size = imageSize;
    bufferSettings.Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    CreateBuffer(bufferSettings, stagingBuffer, stagingBufferMemory);
        
    void* data;
    vkMapMemory(m_Context->GetDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(m_Context->GetDevice(), stagingBufferMemory);

    stbi_image_free(pixels);

    CreateImageSettings imageSettings{};
    imageSettings.Width = static_cast<uint32_t>(texWidth);
    imageSettings.Height = static_cast<uint32_t>(texHeight);
    imageSettings.Format = VK_FORMAT_R8G8B8A8_SRGB;
    imageSettings.Tiling = VK_IMAGE_TILING_OPTIMAL;
    imageSettings.Usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageSettings.Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    CreateImage(imageSettings, m_TextureImage, m_TextureImageMemory);

    TransitionImageLayout(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        CopyBufferToImage(stagingBuffer, m_TextureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    TransitionImageLayout(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(m_Context->GetDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_Context->GetDevice(), stagingBufferMemory, nullptr);
    return true;
}

bool VulkanBuffer::CreateTextureImageView() {
    return CreateTextureImageView(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB);
}

bool VulkanBuffer::CreateTextureImageView(VkImage image_, VkFormat format_) {
    return m_Swapchain->CreateImageView(image_, format_, VK_IMAGE_ASPECT_COLOR_BIT, m_TextureImageView);
}

bool VulkanBuffer::CreateTextureSampler() {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(m_Context->GetPhysicalDevice(), &properties);

    VkSamplerCreateInfo samplerCI{};
    samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCI.magFilter = VK_FILTER_LINEAR;
    samplerCI.minFilter = VK_FILTER_LINEAR;
    samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCI.anisotropyEnable = VK_TRUE;
    samplerCI.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerCI.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerCI.unnormalizedCoordinates = VK_FALSE;
    samplerCI.compareEnable = VK_FALSE;
    samplerCI.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCI.mipLodBias = 0.f;
    samplerCI.minLod = 0.f;
    samplerCI.maxLod = 0.f;

    return (vkCreateSampler(m_Context->GetDevice(), &samplerCI, nullptr, &m_Sampler) == VK_SUCCESS);
}

bool VulkanBuffer::CreateVertexBuffer() {
    VkDeviceSize bufferSize = static_cast<uint64_t>(sizeof(vertices[0]) * vertices.size());

    VkBuffer stagingBuffer{VK_NULL_HANDLE};
    VkDeviceMemory stagingBufferMemory{VK_NULL_HANDLE};
    CreateBufferSettings bufferSettings{};
    bufferSettings.Properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    bufferSettings.Size = bufferSize;
    bufferSettings.Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    CreateBuffer(bufferSettings, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(m_Context->GetDevice(), stagingBufferMemory, 0, sizeof(vertices[0]) * vertices.size(), 0, &data);
        memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(m_Context->GetDevice(), stagingBufferMemory);

    bufferSettings.Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    bufferSettings.Size = bufferSize;
    bufferSettings.Usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    CreateBuffer(bufferSettings, m_VertexBuffer, m_VertexBufferMemory);
    
    CopyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);

    vkDestroyBuffer(m_Context->GetDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_Context->GetDevice(), stagingBufferMemory, nullptr);
    return true;
}

bool VulkanBuffer::CreateIndexBuffer() {
    VkDeviceSize bufferSize = static_cast<uint64_t>(sizeof(indices[0]) * indices.size());

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBufferSettings bufferSettings{};
    bufferSettings.Properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    bufferSettings.Size = bufferSize;
    bufferSettings.Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    CreateBuffer(bufferSettings, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(m_Context->GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(m_Context->GetDevice(), stagingBufferMemory);

    bufferSettings.Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    bufferSettings.Size = bufferSize;
    bufferSettings.Usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    CreateBuffer(bufferSettings, m_IndexBuffer, m_IndexBufferMemory);

    CopyBuffer(stagingBuffer, m_IndexBuffer, bufferSize);

    vkDestroyBuffer(m_Context->GetDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_Context->GetDevice(), stagingBufferMemory, nullptr);
    return true;
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
        CreateBuffer(bufferSettings, m_UniformBuffers[i], m_UniformBuffersMemory[i]);
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

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_TextureImageView;
        imageInfo.sampler = m_Sampler;

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

void VulkanBuffer::CleanupDepthResources() {
    vkDestroyImageView(m_Context->GetDevice(), m_DepthImageView, nullptr);
    vkDestroyImage(m_Context->GetDevice(), m_DepthImage, nullptr);
    vkFreeMemory(m_Context->GetDevice(), m_DepthImageMemory, nullptr);
}

void VulkanBuffer::CleanupTextrueSampler()
{
    vkDestroySampler(m_Context->GetDevice(), m_Sampler, nullptr);
    vkDestroyImageView(m_Context->GetDevice(), m_TextureImageView, nullptr);
    vkDestroyImage(m_Context->GetDevice(), m_TextureImage, nullptr);
    vkFreeMemory(m_Context->GetDevice(), m_TextureImageMemory, nullptr);
}

void VulkanBuffer::FreeMemory()
{
    // TODO: split this function
    for (auto&& memory : m_UniformBuffersMemory) {
        vkFreeMemory(m_Context->GetDevice(), memory, nullptr);
    }
    vkFreeMemory(m_Context->GetDevice(), m_IndexBufferMemory, nullptr);
    vkFreeMemory(m_Context->GetDevice(), m_VertexBufferMemory, nullptr);
}

void VulkanBuffer::CleanupVertexBuffer() {
    // TODO: split this function
    for (auto&& buffer : m_UniformBuffers) {
        vkDestroyBuffer(m_Context->GetDevice(), buffer, nullptr);
    }
    vkDestroyBuffer(m_Context->GetDevice(), m_IndexBuffer, nullptr);
    vkDestroyBuffer(m_Context->GetDevice(), m_VertexBuffer, nullptr);
}

void VulkanBuffer::CleanupAll() {
    CleanupDepthResources();
    CleanupTextrueSampler();
    vkDestroyDescriptorPool(m_Context->GetDevice(), m_DescriptorPool, nullptr);
    CleanupVertexBuffer();
    FreeMemory();
}

uint32_t VulkanBuffer::GetIndicesCount() const {
    return static_cast<uint32_t>(indices.size());
}

bool VulkanBuffer::CreateImage(const CreateImageSettings &imageSettings_, VkImage& image_, VkDeviceMemory& imageMemory_) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = imageSettings_.Width;
    imageInfo.extent.height = imageSettings_.Height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    // TODO: use supported by GPU format
    imageInfo.format = imageSettings_.Format;
    imageInfo.tiling = imageSettings_.Tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = imageSettings_.Usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0;

    vkCreateImage(m_Context->GetDevice(), &imageInfo, nullptr, &image_);

    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(m_Context->GetDevice(), image_, &memReq);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = m_Context->FindMemoryType(memReq.memoryTypeBits, imageSettings_.Properties);

    vkAllocateMemory(m_Context->GetDevice(), &allocInfo, nullptr, &imageMemory_);

    return vkBindImageMemory(m_Context->GetDevice(), image_, imageMemory_, 0) == VK_SUCCESS;
}

bool VulkanBuffer::CreateBuffer(const CreateBufferSettings &bufferSettings_, VkBuffer& buffer_, VkDeviceMemory& bufferMemory_) {
    VkBufferCreateInfo bufferCI{};
    bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCI.size = bufferSettings_.Size;
    bufferCI.usage = bufferSettings_.Usage;
    bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_Context->GetDevice(), &bufferCI, nullptr, &buffer_) != VK_SUCCESS) {
        return false;
    }

    VkMemoryRequirements memRequirements{};
    vkGetBufferMemoryRequirements(m_Context->GetDevice(), buffer_, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = m_Context->FindMemoryType(memRequirements.memoryTypeBits, bufferSettings_.Properties);
    if (allocInfo.memoryTypeIndex == UINT32_MAX) {
        return false;
    }

    vkAllocateMemory(m_Context->GetDevice(), &allocInfo, nullptr, &bufferMemory_);

    return vkBindBufferMemory(m_Context->GetDevice(), buffer_, bufferMemory_, 0) == VK_SUCCESS;
}

void VulkanBuffer::CopyBuffer(VkBuffer srcBuffer_, VkBuffer dstBuffer_, VkDeviceSize size_) {
    VkCommandBuffer commandBuffer = m_FrameBuffer->BeginSingleTimeCommands();
    {
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size_;

        vkCmdCopyBuffer(commandBuffer, srcBuffer_, dstBuffer_, 1, &copyRegion);
    }
    m_FrameBuffer->EndSingleTimeCommands(commandBuffer);
}

void VulkanBuffer::TransitionImageLayout(VkImage image_, VkFormat format_
    , VkImageLayout oldLayout_, VkImageLayout newLayout_) {
    (void)format_;
    VkCommandBuffer commandBuffer = m_FrameBuffer->BeginSingleTimeCommands();
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout_;
        barrier.newLayout = newLayout_;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image_;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0; // TODO
        barrier.dstAccessMask = 0; // TODO

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout_ == VK_IMAGE_LAYOUT_UNDEFINED && newLayout_ == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout_ == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout_ == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    }
    m_FrameBuffer->EndSingleTimeCommands(commandBuffer);
}

void VulkanBuffer::CopyBufferToImage(VkBuffer buffer_, VkImage image_, uint32_t width_, uint32_t height_) {
    VkCommandBuffer commandBuffer = m_FrameBuffer->BeginSingleTimeCommands();
    {
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            width_,
            height_,
            1
        };

        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer_,
            image_,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );
    }
    m_FrameBuffer->EndSingleTimeCommands(commandBuffer);
}

VkFormat VulkanBuffer::FindSupportedFormat(const std::vector<VkFormat>& candidates_, VkImageTiling tiling_, VkFormatFeatureFlags features_) {
    for (auto&& format : candidates_) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_Context->GetPhysicalDevice(), format, &props);

        if (tiling_ == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features_) == features_) {
            return format;
        }

        if (tiling_ == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features_) == features_) {
            return format;
        }
    }

    throw std::runtime_error("Filed to find supported formt!");
}

bool VulkanBuffer::HasStencilComponent(VkFormat format_) {
    return format_ == VK_FORMAT_D32_SFLOAT_S8_UINT || format_ == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkFormat VulkanBuffer::FindDepthFormat() {
    return FindSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

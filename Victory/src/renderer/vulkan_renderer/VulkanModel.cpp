#include <vulkan/vulkan.h>
#include <string>

#include "VulkanModel.h"

#include "VulkanContext.h"
#include "VulkanFrameBuffer.h"

#include <unordered_map>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

VulkanModel::VulkanModel(VulkanContext *context_, VulkanFrameBuffer *frameBuffer_) 
    : m_Context{context_}, m_FrameBuffer{frameBuffer_} {
}

bool VulkanModel::LoadModel(std::string &&path_) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path_.c_str())) {
        throw std::runtime_error(warn + err);
    }

    std::unordered_map<VertexData, uint32_t> uniqueVertices;
    uniqueVertices.reserve(attrib.vertices.size());

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

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(m_Vertices.size());
                m_Vertices.emplace_back(vertex);
            }

            m_Indices.emplace_back(uniqueVertices[vertex]);
        }
    }

    return true;
}

bool VulkanModel::CreateVertexBuffer() {
    VkDeviceSize bufferSize = static_cast<uint64_t>(sizeof(m_Vertices[0]) * m_Vertices.size());

    VkBuffer stagingBuffer{VK_NULL_HANDLE};
    VkDeviceMemory stagingBufferMemory{VK_NULL_HANDLE};
    CreateBufferSettings bufferSettings{};
    bufferSettings.Properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    bufferSettings.Size = bufferSize;
    bufferSettings.Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    BindBuffer(m_Context, bufferSettings, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(m_Context->GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, m_Vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(m_Context->GetDevice(), stagingBufferMemory);

    bufferSettings.Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    bufferSettings.Size = bufferSize;
    bufferSettings.Usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    BindBuffer(m_Context, bufferSettings, m_VertexBuffer, m_VertexBufferMemory);
    
    CopyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);

    vkDestroyBuffer(m_Context->GetDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_Context->GetDevice(), stagingBufferMemory, nullptr);
    return true;
}

bool VulkanModel::CreateIndexBuffer() {
    VkDeviceSize bufferSize = static_cast<uint64_t>(sizeof(m_Indices[0]) * m_Indices.size());

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBufferSettings bufferSettings{};
    bufferSettings.Properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    bufferSettings.Size = bufferSize;
    bufferSettings.Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    BindBuffer(m_Context, bufferSettings, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(m_Context->GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, m_Indices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(m_Context->GetDevice(), stagingBufferMemory);

    bufferSettings.Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    bufferSettings.Size = bufferSize;
    bufferSettings.Usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    BindBuffer(m_Context, bufferSettings, m_IndexBuffer, m_IndexBufferMemory);

    CopyBuffer(stagingBuffer, m_IndexBuffer, bufferSize);

    vkDestroyBuffer(m_Context->GetDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_Context->GetDevice(), stagingBufferMemory, nullptr);
    return true;
}

void VulkanModel::CleanupBuffer(VkBuffer &buffer_) {
    vkDestroyBuffer(m_Context->GetDevice(), buffer_, nullptr);
}

void VulkanModel::FreeBufferMemory(VkDeviceMemory &memory_) {
    vkFreeMemory(m_Context->GetDevice(), memory_, nullptr);
}

void VulkanModel::CleanupAll() {
    CleanupBuffer(m_IndexBuffer);
    CleanupBuffer(m_VertexBuffer);
    FreeBufferMemory(m_IndexBufferMemory);
    FreeBufferMemory(m_VertexBufferMemory);
}

//----------------------------------------------------------------------
//--------------------------------PRIVATE-------------------------------
//----------------------------------------------------------------------

void VulkanModel::CopyBuffer(VkBuffer srcBuffer_, VkBuffer dstBuffer_, VkDeviceSize size_)
{
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

bool BindBuffer(const VulkanContext* context_, const CreateBufferSettings &bufferSettings_, 
        VkBuffer &buffer_, VkDeviceMemory &bufferMemory_) {
    VkBufferCreateInfo bufferCI{};
    bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCI.size = bufferSettings_.Size;
    bufferCI.usage = bufferSettings_.Usage;
    bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(context_->GetDevice(), &bufferCI, nullptr, &buffer_) != VK_SUCCESS) {
        return false;
    }

    VkMemoryRequirements memRequirements{};
    vkGetBufferMemoryRequirements(context_->GetDevice(), buffer_, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = context_->FindMemoryType(memRequirements.memoryTypeBits, bufferSettings_.Properties);
    if (allocInfo.memoryTypeIndex == UINT32_MAX) {
        return false;
    }

    vkAllocateMemory(context_->GetDevice(), &allocInfo, nullptr, &bufferMemory_);

    return vkBindBufferMemory(context_->GetDevice(), buffer_, bufferMemory_, 0) == VK_SUCCESS;
}


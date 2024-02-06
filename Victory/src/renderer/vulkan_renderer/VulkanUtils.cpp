#include <vulkan/vulkan.h>

#include "VulkanUtils.h"

#include "VulkanContext.h"
#include <string>

void VulkanUtils::LoadModel(std::string&& path_)
{
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

    CreateVertexBuffer();
    CreateIndexBuffer();
}


bool VulkanUtils::CreateSemaphore(VkDevice device_, VkSemaphore *semaphore_)
{
    if (!semaphore_) {
        return false;
    }

    VkSemaphoreCreateInfo semaphoreCI{};
    semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    return vkCreateSemaphore(device_, &semaphoreCI, nullptr, semaphore_) == VK_SUCCESS;
}

bool VulkanUtils::CreateFence(VkDevice device_, VkFence* fence_) {
    if (!fence_) {
        return false;
    }

    VkFenceCreateInfo fenceCI{};
    fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    return vkCreateFence(device_, &fenceCI, nullptr, fence_) == VK_SUCCESS;
}

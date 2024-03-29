#include <string>
#include <vector>
#include <array>
#include <vulkan/vulkan.h>
#include "VertexData.h"

#include "VulkanModel.h"

#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanImage.h"
#include "VulkanFileUtils.h"

namespace Victory
{
    VulkanModel::VulkanModel()
    {
        m_VulkanDevice = VulkanDevice::Init();
        m_VulkanSwapchain = VulkanSwapchain::Init();
        m_Image = new VulkanImage(m_VulkanDevice);
    }

    VulkanModel::~VulkanModel()
    {
    }

    void VulkanModel::LoadModel(const std::string& path_)
    {
        Victory::LoadModel(path_, m_Vertices, m_Indices);

        CreateVertexBuffer();
        CreateIndexBuffer();
    }

    void VulkanModel::CreateVertexBuffer() 
    {
        VkDevice device{ m_VulkanDevice->GetDevice() };
        VkDeviceSize bufferSize = static_cast<uint64_t>(sizeof(m_Vertices[0]) * m_Vertices.size());

        VkBuffer stagingBuffer{VK_NULL_HANDLE};
        VkDeviceMemory stagingBufferMemory{VK_NULL_HANDLE};
        CreateBufferSettings bufferSettings{};
        bufferSettings.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        bufferSettings.size = bufferSize;
        bufferSettings.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        BindBuffer(bufferSettings, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, m_Vertices.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(device, stagingBufferMemory);

        bufferSettings.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        bufferSettings.size = bufferSize;
        bufferSettings.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        BindBuffer(bufferSettings, m_VertexBuffer, m_VertexBufferMemory);
    
        CopyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void VulkanModel::CreateIndexBuffer() 
    {
        VkDevice device{ m_VulkanDevice->GetDevice() };
        VkDeviceSize bufferSize = static_cast<uint64_t>(sizeof(m_Indices[0]) * m_Indices.size());

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        CreateBufferSettings bufferSettings{};
        bufferSettings.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        bufferSettings.size = bufferSize;
        bufferSettings.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        BindBuffer(bufferSettings, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, m_Indices.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(device, stagingBufferMemory);

        bufferSettings.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        bufferSettings.size = bufferSize;
        bufferSettings.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        BindBuffer(bufferSettings, m_IndexBuffer, m_IndexBufferMemory);

        CopyBuffer(stagingBuffer, m_IndexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void VulkanModel::LoadTexture(const std::string& path_, VkImageCreateInfo& imageCI_)
    {
        VkDevice device{ m_VulkanDevice->GetDevice() };
        int texWidth, texHeight;

        unsigned char* pixels{ Victory::LoadPixels(path_, texWidth, texHeight) };

        VkDeviceSize imageSize = static_cast<uint32_t>(texWidth) * static_cast<uint32_t>(texHeight) * 4;
        VkBuffer stagingBuffer{VK_NULL_HANDLE};
        VkDeviceMemory stagingBufferMemory{VK_NULL_HANDLE};
        CreateBufferSettings bufferSettings{};
        bufferSettings.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        bufferSettings.size = imageSize;
        bufferSettings.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        BindBuffer(bufferSettings, stagingBuffer, stagingBufferMemory);
        
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
            memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(device, stagingBufferMemory);

        Victory::DeletePixels(pixels);

        imageCI_.extent.width = static_cast<uint32_t>(texWidth);
        imageCI_.extent.height = static_cast<uint32_t>(texHeight);
        imageCI_.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        m_Image->CreateImage(imageCI_, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        m_Image->TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        CopyBufferToImage(stagingBuffer, imageCI_);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);

        m_Image->CreateImageView(imageCI_.format, VK_IMAGE_ASPECT_COLOR_BIT);

        CreateSampler();

        m_Image->GenerateMipmaps(imageCI_.format);
    }

    void VulkanModel::CreateDescriptors(VkDescriptorSetLayout layout_, VkDescriptorBufferInfo bufferI_)
    {
        CreateDescriptorPool();
        CreateDescriptorSets(1, layout_, bufferI_);
    }

    void VulkanModel::CleanupAll()
    {
        VkDevice device{ m_VulkanDevice->GetDevice() };

        vkFreeMemory(device, m_VertexBufferMemory, nullptr);
        vkFreeMemory(device, m_IndexBufferMemory, nullptr);

        vkDestroyBuffer(device, m_VertexBuffer, nullptr);
        vkDestroyBuffer(device, m_IndexBuffer, nullptr);

        m_Image->CleanupAll();
        delete m_Image;

        vkDestroySampler(device, m_ImageSampler, nullptr);
        vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
    }

    void VulkanModel::CreateSampler()
    {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(m_VulkanDevice->GetPhysicalDevice(), &properties);

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
        samplerCI.minLod = 0.f;
        // samplerCI.maxLod = static_cast<float>(m_MipLevels);
        samplerCI.maxLod = 1.f;
        samplerCI.mipLodBias = 0.f;

        vkCreateSampler(m_VulkanDevice->GetDevice(), &samplerCI, nullptr, &m_ImageSampler);
    }

    void VulkanModel::BindBuffer(const CreateBufferSettings &bufferSettings_, 
        VkBuffer& buffer_, VkDeviceMemory& bufferMemory_) 
    {
        VkDevice device{ m_VulkanDevice->GetDevice() };
        VkBufferCreateInfo bufferCI{};
        bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCI.size = bufferSettings_.size;
        bufferCI.usage = bufferSettings_.usage;
        bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        vkCreateBuffer(device, &bufferCI, nullptr, &buffer_);

        VkMemoryRequirements memRequirements{};
        vkGetBufferMemoryRequirements(device, buffer_, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = m_VulkanDevice->FindMemoryType(memRequirements.memoryTypeBits, bufferSettings_.properties);

        vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory_);

        vkBindBufferMemory(device, buffer_, bufferMemory_, 0);
    }

    void VulkanModel::CopyBuffer(VkBuffer srcBuffer_, VkBuffer dstBuffer_, VkDeviceSize size_)
    {
        VkCommandBuffer commandBuffer = m_VulkanDevice->BeginSingleTimeCommands();
        {
            VkBufferCopy copyRegion{};
            copyRegion.srcOffset = 0;
            copyRegion.dstOffset = 0;
            copyRegion.size = size_;

            vkCmdCopyBuffer(commandBuffer, srcBuffer_, dstBuffer_, 1, &copyRegion);
        }
        m_VulkanDevice->EndSingleTimeCommands(commandBuffer);
    }

    void VulkanModel::CopyBufferToImage(VkBuffer stagingBuffer_, const VkImageCreateInfo& imageCI_) {
        VkCommandBuffer commandBuffer = m_VulkanDevice->BeginSingleTimeCommands();
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
                imageCI_.extent.width,
                imageCI_.extent.height,
                1
            };

            vkCmdCopyBufferToImage(commandBuffer, stagingBuffer_, m_Image->GetImage(),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        }
        m_VulkanDevice->EndSingleTimeCommands(commandBuffer);
    }

    void VulkanModel::CreateDescriptorPool()
    {
        std::array<VkDescriptorPoolSize, 2> poolSize{};
        poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize[0].descriptorCount = 1;
        // poolSize[0].descriptorCount = static_cast<uint32_t>(maxFrames_);
        poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize[1].descriptorCount = 1;
        // poolSize[1].descriptorCount = static_cast<uint32_t>(maxFrames_);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSize.size());
        poolInfo.pPoolSizes = poolSize.data();
        poolInfo.maxSets = 1;
        // poolInfo.maxSets = static_cast<uint32_t>(maxFrames_);

        vkCreateDescriptorPool(m_VulkanDevice->GetDevice(), &poolInfo, nullptr, &m_DescriptorPool);
    }

    void VulkanModel::CreateDescriptorSets(uint32_t maxFrames_, VkDescriptorSetLayout layout_, VkDescriptorBufferInfo bufferI_) 
    {
        std::vector<VkDescriptorSetLayout> layouts(maxFrames_, layout_);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_DescriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(maxFrames_);
        allocInfo.pSetLayouts = layouts.data();

        vkAllocateDescriptorSets(m_VulkanDevice->GetDevice(), &allocInfo, &m_DescriptorSet);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_Image->GetImageView();
        imageInfo.sampler = m_ImageSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = m_DescriptorSet;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferI_;
        descriptorWrites[0].pImageInfo = nullptr;
        descriptorWrites[0].pTexelBufferView = nullptr;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = m_DescriptorSet;
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(m_VulkanDevice->GetDevice(), static_cast<uint32_t>(descriptorWrites.size())
            , descriptorWrites.data(), 0, nullptr);
    }

}

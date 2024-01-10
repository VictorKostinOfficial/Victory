#pragma once

struct CreateBufferSettings;
struct VertexData;

namespace Victory 
{
    struct CreateBufferSettings {
        VkDeviceSize size;
        VkBufferUsageFlags usage;
        VkMemoryPropertyFlags properties;
    };

    class VulkanDevice;
    class VulkanSwapchain;
    class VulkanImage;

    class VulkanModel
    {
    public:
        VulkanModel();
        ~VulkanModel();

        void LoadModel(std::string &&path);
        void LoadTexture(std::string &&path, VkImageCreateInfo& imageCI_);
        void CreateDescriptors(VkDescriptorSetLayout layout_, VkDescriptorBufferInfo bufferI_);

        void CleanupAll();

        inline const VkBuffer& GetVertexBuffer() const
        {
            return m_VertexBuffer;
        }

        inline const VkBuffer& GetIndexBuffer() const
        {
            return m_IndexBuffer;
        }

        inline const std::vector<VertexData>& GetVerices() const
        {
            return m_Vertices;
        }

        inline const std::vector<uint16_t>& GetIndices() const
        {
            return m_Indices;
        }

        inline const VkDescriptorSet& GetDescriptorSet() const 
        {
            return m_DescriptorSet;
        }

    private:

        void CreateVertexBuffer();
        void CreateIndexBuffer();

        void CreateSampler();

        // TODO: Split this functions
        void BindBuffer(const CreateBufferSettings& bufferSettings_,
            VkBuffer& buffer_, VkDeviceMemory& bufferMemory_);
        void CopyBuffer(VkBuffer srcBuffer_, VkBuffer dstBuffer_, VkDeviceSize size_);
        void CopyBufferToImage(VkBuffer stagingBuffer_, const VkImageCreateInfo& imageCI_);

        void CreateDescriptorPool();
        void CreateDescriptorSets(uint32_t maxFrames_, VkDescriptorSetLayout layout_, VkDescriptorBufferInfo bufferI_);

    private:

        VulkanDevice* m_VulkanDevice;
        VulkanSwapchain* m_VulkanSwapchain;

        std::vector<VertexData> m_Vertices;
        std::vector<uint16_t> m_Indices;

        VkBuffer m_VertexBuffer{ VK_NULL_HANDLE };
        VkDeviceMemory m_VertexBufferMemory{ VK_NULL_HANDLE };
        VkBuffer m_IndexBuffer{ VK_NULL_HANDLE };
        VkDeviceMemory m_IndexBufferMemory{ VK_NULL_HANDLE };

        VulkanImage* m_Image;
        VkSampler m_ImageSampler{ VK_NULL_HANDLE };

        VkDescriptorPool m_DescriptorPool;
        VkDescriptorSet m_DescriptorSet;
    };
}
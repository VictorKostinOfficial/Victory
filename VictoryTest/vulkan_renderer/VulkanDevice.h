#pragma once

namespace Victory {
    enum class QueueIndex 
    {
        eGraphics,
        ePresent,
        eCompute,
        eTransfer
    };

    struct VulkanQueueIndices 
    {
        uint32_t graphicsQueueIndex{ UINT32_MAX };
        uint32_t presentQueueIndex{ UINT32_MAX };
        uint32_t computeQueueIndex{ UINT32_MAX };
        uint32_t transferQueueIndex{ UINT32_MAX };

        const uint32_t& operator[](QueueIndex queueIndex_) const 
        {
            switch (queueIndex_) 
            {
            case QueueIndex::eGraphics:
                return graphicsQueueIndex;
            case QueueIndex::ePresent:
                return presentQueueIndex;
            case QueueIndex::eCompute:
                return computeQueueIndex;
            case QueueIndex::eTransfer:
                return transferQueueIndex;
            default:
                return dummy;
            }
        }
    private:
        uint32_t dummy{ UINT16_MAX };
    };

    class VulkanDevice 
    {
    public:

        static VulkanDevice* Init();
        static void Cleanup();

        void CreateInstance();
        void PickPhysicalDevice(VkSurfaceKHR surface_);
        void CreateLogicalDevice();

        const uint32_t FindMemoryType(const uint32_t typeFilter_, 
            const VkMemoryPropertyFlags memoryProperty_) const;
        
        const VkFormat FindDepthFormat();
        const VkFormat FindSupportedFormat(const std::vector<VkFormat> &formats, 
            VkImageTiling tiling_, VkFormatFeatureFlags features_);

        VkCommandBuffer BeginSingleTimeCommands();
        void EndSingleTimeCommands(VkCommandBuffer commandBuffer_);

        inline const VkInstance GetInstance() const 
        {
            return m_Instance;
        }

        inline const VkPhysicalDevice GetPhysicalDevice() const 
        {
            return m_PhysicalDevice;
        }

        inline const VkDevice GetDevice() const 
        {
            return m_Device;
        }

        inline const VulkanQueueIndices& GetQueueIndices() const 
        {
            return m_QueueIndices;
        }

        inline const uint32_t GetQueueIndex(QueueIndex index_) const 
        {
            return m_QueueIndices[index_];
        }

        inline void GetQueue(VkQueue& queue_, const QueueIndex queueIndex_) const 
        {
            vkGetDeviceQueue(m_Device, m_QueueIndices[queueIndex_], 0, &queue_);
        }

        inline VkSampleCountFlagBits GetMaxSampleCount() const 
        {
            return m_MaxSampleCount;
        }

    private:

        VulkanDevice() = default;
        ~VulkanDevice() = default;

        void CleanupResourses();

        uint32_t RateDeviceSuitability(VkPhysicalDevice phDevice_);
        bool PickQueueIndecies(VkPhysicalDevice phDevice_, VkSurfaceKHR surface_);
        void DefineMaxSampleCount();

    private:

        VkInstance m_Instance{ VK_NULL_HANDLE };
        VkPhysicalDevice m_PhysicalDevice{ VK_NULL_HANDLE };
        VkDevice m_Device{ VK_NULL_HANDLE };

        VulkanQueueIndices m_QueueIndices;

        VkCommandPool m_CommandPool;

        VkSampleCountFlagBits m_MaxSampleCount{ VK_SAMPLE_COUNT_1_BIT };
    };
}

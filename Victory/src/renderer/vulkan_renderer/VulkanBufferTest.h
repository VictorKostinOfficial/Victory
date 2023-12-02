#pragma once

class VulkanContext;
class VulkanFrameBuffer;

struct CreateBufferSettings {
    VkDeviceSize Size;
    VkBufferUsageFlags Usage;
    VkMemoryPropertyFlags Properties;
};

class BindBuffer {
public:

    BindBuffer(VulkanContext* context_, VulkanFrameBuffer* frameBuffer_);

    void BindBufferMemory(const CreateBufferSettings &bufferSettings_, 
        VkBuffer &buffer_, VkDeviceMemory &bufferMemory_);

protected:

    VulkanContext* m_Context;
    VulkanFrameBuffer* m_FrameBuffer;
};

template<class Buffer>
class VulkanTempTest : public BindBuffer {
public:

    VulkanTempTest(VulkanContext* context_, VulkanFrameBuffer* frameBuffer_);
    virtual ~VulkanTempTest();


protected:

    Buffer m_Buffer{VK_NULL_HANDLE};
    VkDeviceMemory m_DeviceMemory{VK_NULL_HANDLE};
};

class VulkanBufferTest : public VulkanTempTest<VkBuffer> {

};

class VulkanImageTest : public VulkanTempTest<VkImage> {
public:

    VulkanImageTest(VulkanContext* context_, VulkanFrameBuffer* frameBuffer_);
    virtual ~VulkanImageTest();

private:

    VkImageView m_ImageView;
    VkSampler m_Sampler;
};

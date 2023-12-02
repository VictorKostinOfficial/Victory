#pragma once

#include "../Renderer.h"

#include <vector>

struct VkSemaphore_T;
struct VkFence_T;
typedef VkSemaphore_T *VkSemaphore;
typedef VkFence_T *VkFence;

class VulkanContext;
class VulkanSwapchain;
class VulkanPipeline;
class VulkanFrameBuffer;
class VulkanImage;
class VulkanModel;
class VulkanBuffer;

struct GLFWwindow;

class VulkanRenderer final : public Renderer {
public:

    static Renderer* CreateRenderer();

    virtual void Initialize(const char* applicationName_) override;

    virtual bool IsRunning() override;
    virtual void PollEvents() override;

    virtual bool Resize() override;
    virtual void BeginFrame() override;
    virtual void RecordCommandBuffer() override;
    virtual void EndFrame() override;

    virtual void Destroy() override;

    void SetIsResized(bool isResized_);

private:

    VulkanRenderer() = default;
    virtual ~VulkanRenderer() = default;

    void RecreateSwapchain();

private: 

    GLFWwindow* m_Window;

    VulkanContext* m_VulkanContext;
    VulkanSwapchain* m_VulkanSwapchain;
    VulkanPipeline* m_VulkanPipeline;
    VulkanFrameBuffer* m_VulkanFrameBuffer;
    // VulkanBuffer* m_VulkanBuffer;

    uint32_t m_CurrentFrame{0};
    uint32_t m_ImageIndex{0};

    const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    bool m_IsResized{false};

    std::vector<VulkanImage> m_Images;
    std::vector<VulkanModel> m_Models;
    std::vector<VulkanBuffer> m_Buffers;

    // TODO: create memoryMenegmentClass or move to VulkanSynchronization
    std::vector<VkSemaphore> m_AvailableSemaphores;
    std::vector<VkSemaphore> m_FinishedSemaphores;
    std::vector<VkFence> m_InFlightFences;
};
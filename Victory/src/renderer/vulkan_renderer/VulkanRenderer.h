#pragma once

#include "../Renderer.h"

#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanPipeline.h"
#include "VulkanFrameBuffer.h"

struct GLFWwindow;

class VulkanRenderer final : public Renderer {
public:

    static Renderer* CreateRenderer();

    virtual void Initialize(const char* applicationName_) override;

    virtual bool IsRunning() override;
    virtual void PollEvents() override;

    virtual void Resize() override;
    virtual void BeginFrame() override;
    virtual void EndFrame() override;

    virtual void Destroy() override;

private:

    VulkanRenderer() = default;
    virtual ~VulkanRenderer() = default;

private: 

    GLFWwindow* m_Window;

    VulkanContext m_VulkanContext;
    VulkanSwapchain m_VulkanSwapchain;
    VulkanPipeline m_VulkanPipeline;
    VulkanFrameBuffer m_VulkanFrameBuffer;

    uint32_t m_CurrentFrame{0};
    const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    std::vector<VkSemaphore> m_AvailableSemaphores;
    std::vector<VkSemaphore> m_FinishedSemaphores;
    std::vector<VkFence> m_InFlightFences;
};
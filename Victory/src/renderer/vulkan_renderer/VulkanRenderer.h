#pragma once

#include "../Renderer.h"

#include "vulkan/vulkan.hpp"

struct GLFWwindow;

class VulkanContext;
class VulkanSwapchain;
class VulkanPipeline;
class VulkanFrameBuffer;

class VulkanRenderer : public Renderer {
public:

    VulkanRenderer();
    virtual ~VulkanRenderer();

    virtual bool IsRunning() override;
    virtual void PollEvents() override;

    virtual void Resize() override;
    virtual void BeginFrame() override;
    virtual void EndFrame() override;
    virtual void Destroy() override;

private:

    virtual bool Initialize(const char* applicationName) override;

private: 

    // TODO: till we include vulkan.hpp here
    // we can save state here and rid of all pointers
    GLFWwindow* m_Window;

    VulkanContext* m_VulkanContext;
    VulkanSwapchain* m_VulkanSwapchain;
    VulkanPipeline* m_VulkanPipeline;
    VulkanFrameBuffer* m_VulkanFrameBuffer;

    uint32_t m_CurrentFrame{0};
    const uint32_t MAX_FRAMES_IN_FLIGHT{2};

    std::vector<vk::Semaphore> m_AvailableSemaphores;
    std::vector<vk::Semaphore> m_FinishedSemaphores;
    std::vector<vk::Fence> m_InFlightFences;
};
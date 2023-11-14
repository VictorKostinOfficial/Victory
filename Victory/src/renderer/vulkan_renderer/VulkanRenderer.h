#pragma once

#include "../Renderer.h"

#include "vulkan/vulkan.hpp"

class GLFWwindow;

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

    vk::Semaphore m_AvailableSemaphore{VK_NULL_HANDLE};
    vk::Semaphore m_FinishedSemaphore{VK_NULL_HANDLE};
    vk::Fence m_InFlightFence{VK_NULL_HANDLE};
};
#pragma once

#include "../Renderer.h"

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

    GLFWwindow* m_Window;

    VulkanContext* m_VulkanContext;
    VulkanSwapchain* m_VulkanSwapchain;
    VulkanPipeline* m_VulkanPipeline;
    VulkanFrameBuffer* m_VulkanFrameBuffer;
};
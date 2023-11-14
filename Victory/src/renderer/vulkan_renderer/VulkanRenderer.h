#pragma once

#include "../Renderer.h"

class GLFWwindow;

class VulkanContext;
class VulkanSwapchain;
class VulkanPipeline;
class VulkanFrameBuffers;

class VulkanRenderer : public Renderer {
public:

    VulkanRenderer();
    virtual ~VulkanRenderer();

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
    VulkanFrameBuffers* m_VulkanFrameBuffers;
};
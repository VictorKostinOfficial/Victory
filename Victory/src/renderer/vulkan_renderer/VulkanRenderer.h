#pragma once

#include "../Renderer.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <unordered_map>

namespace Victory 
{
    class VulkanDevice;
    class VulkanSwapchain;
    class VulkanGraphicsPipeline;
    class VulkanModel;
}

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

    void CreateSemaphores();
    void CleanupSemaphores();
    void SetIsResized(bool value_, int width_, int height_);
    void SetIsRunning(bool value_);

    void RecreateSwapchain();
    bool InitImGui();

    friend void OnWindowClose(GLFWwindow* window_);
    friend void OnWindowResize(GLFWwindow* window_, int width_, int height_);

private: 

    GLFWwindow* m_Window;
    int m_WindowWidth{ 0 };
    int m_WindowHeight{ 0 };

    Victory::VulkanDevice* m_VulkanDevice;
    Victory::VulkanSwapchain* m_VulkanSwapchain;

    bool m_IsRunning{ true };
    bool m_IsResized{ false };

    std::unordered_map<std::string, Victory::VulkanGraphicsPipeline*> m_Pipelines;

    const uint32_t m_MaxImageInFight{ 2 };
    uint32_t m_CurrentFrame{ 0 };
    uint32_t m_ImageIndex{ 0 };

    std::vector<VkCommandBuffer> m_CommandBuffers;
    
    std::vector<VkSemaphore> m_ImageAvailableSemaphore;
    std::vector<VkSemaphore> m_RenderingFinishedSemaphore;
    std::vector<VkFence> m_QueueSubmitFence;
};
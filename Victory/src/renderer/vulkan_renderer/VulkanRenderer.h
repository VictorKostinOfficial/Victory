#pragma once

#include "../Renderer.h"

#include "VulkanContext.h"

class VulkanRenderer : public Renderer {
public:

    VulkanRenderer() = default;
    virtual ~VulkanRenderer();

    virtual void Resize() override;
    virtual void BeginFrame() override;
    virtual void EndFrame() override;
    virtual void Destroy() override;

private:

    virtual void Initialize(const char* applicationName) override;

    VulkanContext m_VulkanContext;
};
#pragma once

#include "../Renderer.h"

class VulkanRenderer : public Renderer {
public:

    VulkanRenderer() = default;
    virtual ~VulkanRenderer();

    virtual void Resize() override;
    virtual void BeginFrame() override;
    virtual void EndFrame() override;

private:

    virtual void Initialize() override;
    virtual void Destroy() override;
};
#pragma once

#include "renderer/GraphicsRenderer.h"

#include "VulkanTypes.h"

class VulkanRenderer : public GraphicsRenderer 
{
public:

    virtual void Initialize();
	virtual void Resize();
    virtual bool BeginFrame();
    virtual bool EndFrame();
    virtual void Cleanup();

    static GraphicsRenderer* CreateInstance();

protected:

    VulkanRenderer() = default;
    virtual ~VulkanRenderer() = default;

private:

    VulkanContext m_VulkanContext;
};
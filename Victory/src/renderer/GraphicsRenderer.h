#pragma once

class GraphicsRenderer
{
public:

    virtual void Initialize() = 0;
	virtual void Resize() = 0;
    virtual bool BeginFrame() = 0;
    virtual bool EndFrame() = 0;
    virtual void Cleanup() = 0;

    static GraphicsRenderer* CreateInstance();
    static void DestroyInstance();

protected:

    GraphicsRenderer() = default;
    virtual ~GraphicsRenderer() = default;

protected:

    static GraphicsRenderer* s_Context;
};
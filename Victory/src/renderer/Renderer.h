#pragma once

class Renderer {
public:

    virtual ~Renderer() = default;

    static Renderer* CreateRenderer(const char* applicationName);

    virtual void Resize() = 0;
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;

protected:

    Renderer() = default;

private:

    virtual bool Initialize(const char* applicationName) = 0;
    virtual void Destroy() = 0;

    static Renderer* s_Instance;
};

#pragma once

class Renderer {
public:

    static Renderer* CreateRenderer();
    static void CleanupRenderer();

    virtual void Initialize(const char* applicationName) = 0;

    virtual bool IsRunning() = 0;
    virtual void PollEvents() = 0;

    virtual bool Resize() = 0;
    virtual void BeginFrame() = 0;
    virtual void RecordCommandBuffer() = 0;
    virtual void EndFrame() = 0;

    virtual void Destroy() = 0;

protected:

    Renderer() = default;
    virtual ~Renderer() = default;
    static Renderer* s_Instance;
};

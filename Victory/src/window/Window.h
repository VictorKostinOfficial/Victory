#pragma once

class Window {
public:
    Window() = default;
    virtual ~Window() = default;

    virtual void Create() = 0;
    virtual void Destroy() = 0;

    virtual bool ShouldClose() = 0;

    virtual void PollEvents() = 0;
};
#pragma once

struct GLFWwindow;
typedef void (* GLFWframebuffersizefun)(GLFWwindow* window, int width, int height);
typedef void (* GLFWwindowclosefun)(GLFWwindow* window);

namespace Victory {

    class Window {
    public:

        static GLFWwindow* Init(void* userPointer_);
        static void SetResizeCallback(GLFWframebuffersizefun callback_);
        static void SetCloseCallback(GLFWwindowclosefun callback_);
        static void PollEvents();
        static void WaitEvents();
        static void Cleanup();

    private:

        Window() = default;
        ~Window() = default;

        void FrameBufferResizeCallback(GLFWwindow *window_, int width_, int height_);
    };
}
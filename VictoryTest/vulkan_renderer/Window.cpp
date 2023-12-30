#include "Window.h"

#include <GLFW/glfw3.h>

#include <iostream>
#include <exception>

namespace Victory{

    static GLFWwindow* s_Window{ nullptr };

    GLFWwindow* Window::Init(void* userPointer_) {
        if (s_Window) {
            return s_Window;
        }

        std::cout << "Init GLFW" << std::endl;
        if (!glfwInit()) {
            throw std::runtime_error("GLFW was not inited");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        s_Window = glfwCreateWindow(1080, 720, "Victory", nullptr, nullptr);
        if (!s_Window) {
            glfwTerminate();
            throw std::runtime_error("GLFW Window was not created!");
        }

        glfwSetWindowUserPointer(s_Window, userPointer_);
        return s_Window;
    }

    void Window::SetResizeCallback(GLFWframebuffersizefun callback_)
    {
        glfwSetFramebufferSizeCallback(s_Window, callback_);
    }

    void Window::SetCloseCallback(GLFWwindowclosefun callback_)
    {
        glfwSetWindowCloseCallback(s_Window, callback_);
    }

    void Window::PollEvents()
    {
        glfwPollEvents();
    }

    void Window::WaitEvents()
    {
        glfwWaitEvents();
    }

    void Window::Cleanup() {
        std::cout << "Cleanup GLFW" << std::endl;
        glfwDestroyWindow(s_Window);
        glfwTerminate();
    }
}
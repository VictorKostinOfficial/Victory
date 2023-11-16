#include "Application.h"

#include <stdio.h>
#include <stdexcept>

#include "renderer/Renderer.h"

namespace Victory {

Application* Application::s_Instance{ nullptr };

Application::Application(const ApplicationSpecification &applicationSpecification)
    : m_ApplicationSpec(applicationSpecification) {

    if (s_Instance) {
        throw std::runtime_error("Application already exists");
    }
    s_Instance = this;
}

Application::~Application() {
}

void Application::Run() {
    Renderer* pRenderer = Renderer::CreateRenderer(m_ApplicationSpec.Name);

    while (pRenderer->IsRunning())
    {
        pRenderer->PollEvents();
        pRenderer->BeginFrame();
        pRenderer->Resize();
        pRenderer->EndFrame();
    }

    // TODO: Make RAII?
    delete pRenderer;
}
    
} // namespace Victory



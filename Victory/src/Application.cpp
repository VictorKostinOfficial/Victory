#include "Application.h"

#include <stdio.h>
#include <stdexcept>

#include "renderer/Renderer.h"

namespace Victory {

Application* Application::s_Instance{ nullptr };
Renderer* Application::s_Renderer{ nullptr };

Application::Application(const ApplicationSpecification &applicationSpecification)
    : m_ApplicationSpec(applicationSpecification) {

    if (s_Instance) {
        throw std::runtime_error("Application already exists");
    }
    
    s_Renderer = Renderer::CreateRenderer();
    s_Instance = this;
}

Application::~Application() {
    Renderer::CleanupRenderer();
}
 
void Application::Run() {
    s_Renderer->Initialize(m_ApplicationSpec.Name);
    while (s_Renderer->IsRunning())
    {
        s_Renderer->PollEvents();
        if (!s_Renderer->Resize()) {
            s_Renderer->BeginFrame();
            s_Renderer->RecordCommandBuffer();
            s_Renderer->EndFrame();
        }
        // break;
    }
    s_Renderer->Destroy();
}
    
} // namespace Victory



#include "Application.h"
#include <stdio.h>

#include "renderer/Renderer.h"

#include <vulkan/vulkan.hpp>

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
    Renderer* renderer = Renderer::CreateInstance();

    // while (true)
    // {
    //     renderer->Resize();
    //     renderer->BeginFrame();
    //     renderer->EndFrame();
    // }

    delete renderer;

    vk::ApplicationInfo applicationInfo{
        m_ApplicationSpec.Name,
        VK_MAKE_VERSION(1,0,0),
        "Victory Engine",
        VK_MAKE_VERSION(1,0,0),
        VK_API_VERSION_1_1
    };

    // TODO: Collect layers and extensions
    std::vector<const char*> layers;
    std::vector<const char*> extensions;

    vk::InstanceCreateInfo instanceCI{{}, 
        &applicationInfo, 
        layers, 
        extensions
    };

    vk::Instance inst = vk::createInstance(instanceCI);


    printf("All Works\n");
    // TODO: Main loop

    inst.destroy();
}
    
} // namespace Victory



#include "Application.h"
#include <stdio.h>

#include <vulkan/vulkan.hpp>

namespace Victory {

Application* Application::s_Instance{ nullptr };

Application::Application(const ApplicationSpecification &applicationSpecification)
    : m_ApplicationSpec(applicationSpecification) {

    if (s_Instance) {

    }
    s_Instance = this;
}

Application::~Application() {
}

void Application::Run() {


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



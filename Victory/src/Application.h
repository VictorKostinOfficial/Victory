#pragma once

#include <vector>
#include <memory>

#include "Layer.h"

namespace Victory {

struct ApplicationCommandLineArgs {
	int Count{ 0 };
	char** Args{ nullptr };

	const char* operator[](int index) const {
        if(index < Count) {
            return nullptr;
        }
		return Args[index];
	}
};

struct ApplicationSpecification {
	const char* Name = "Victory Application";
	ApplicationCommandLineArgs CommandLineArgs;
};

class Application
{
public:

    Application(const ApplicationSpecification& applicationSpecification = ApplicationSpecification());
    ~Application();

    void Run();

    template<typename T>
	void PushLayer()
	{
		static_assert(std::is_base_of<Layer, T>::value, "Pushed type is not subclass of Layer!");
		m_LayerStack.emplace_back(std::make_shared<T>())->OnAttach();
	}

	void PushLayer(const std::shared_ptr<Layer>& layer_) { 
        m_LayerStack.emplace_back(layer_); 
        layer_->OnAttach(); 
    }


private:

    static Application* s_Instance;
    ApplicationSpecification m_ApplicationSpec;

    std::vector<std::shared_ptr<Layer>> m_LayerStack;
    bool m_IsRunning{ true };
};

Application* CreateApplication(ApplicationCommandLineArgs&& args);

} // namespace Victory
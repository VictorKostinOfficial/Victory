#pragma once

#include <string>
#include <vector>
#include <memory>

#include "Layer.h"

struct GLFWwindow;
class GraphicsContext;

int main(int argc, char** argv);

namespace Victory {

    struct ApplicationCommandLineArgs {
		int Count = 0;
		char** Args = nullptr;

		const char* operator[](int index) const
		{
            if(index < Count) {
                abort();
            }
			return Args[index];
		}
	};

	struct ApplicationSpecification {
		std::string Name = "Victory Application";
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

		void PushLayer(const std::shared_ptr<Layer>& layer) { m_LayerStack.emplace_back(layer); layer->OnAttach(); }

    private:
        static Application* s_Instance;
		ApplicationSpecification m_AppSpec;

        friend int ::main(int argc, char** argv);

		GraphicsContext* m_Context;
		GLFWwindow* m_Window;
		int m_FrameBufferWidth;
		int m_FrameBufferHeight;

		std::vector<std::shared_ptr<Layer>> m_LayerStack;
    };
    

    Application* CreateApplication(ApplicationCommandLineArgs&& args);
}
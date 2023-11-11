#pragma once

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

private:

    static Application* s_Instance;
    ApplicationSpecification m_ApplicationSpec;
};

Application* CreateApplication(ApplicationCommandLineArgs&& args);

} // namespace Victory
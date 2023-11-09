#pragma once

#include "Application.h"

extern Victory::Application* Victory::CreateApplication(ApplicationCommandLineArgs&& args);

int main(int argc, char** argv) {
	auto&& app = Victory::CreateApplication({argc, argv});
	app->Run();
	delete app;

	return 0;
}
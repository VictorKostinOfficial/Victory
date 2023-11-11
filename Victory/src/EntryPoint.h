#pragma once

#include "Application.h"

int main(int argc, char** argv) {

    auto&& app = Victory::CreateApplication({argc, argv});
    app->Run();
    delete app;

    return 0;
}
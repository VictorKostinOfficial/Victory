cmake_minimum_required(VERSION 3.0.0)
project(Sandbox)

find_package(Vulkan REQUIRED)

file(GLOB_RECURSE sources
  src/*.cpp
)

add_executable(Sandbox ${sources})
set_target_properties(Sandbox
    PROPERTIES
        CXX_STANDARD 20
)

target_include_directories(Sandbox PRIVATE ../Victory/src ../Victory/extern/imgui ../Victory/extern/imgui/backends ../Victory/src/renderer/vulkan_renderer)
target_link_libraries(Sandbox PRIVATE Victory Vulkan::Vulkan glfw imgui)
cmake_minimum_required(VERSION 3.0.0)

project(VictoryTest)
set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE sources
    *.cpp
)

find_package(Vulkan REQUIRED)

add_executable(VictoryTest ${sources})
add_dependencies(VictoryTest Shaders)

target_include_directories(VictoryTest PRIVATE ../Victory/externs/stb_image ../Victory/externs/tiny_obj_loader externs/imgui)
target_link_libraries(VictoryTest PRIVATE Vulkan::Vulkan glfw glm)

if(UNIX)
target_compile_options(VictoryTest PRIVATE
    -Wall -Wextra -Wpedantic -Werror 
)
endif()

cmake_minimum_required(VERSION 3.0.0)

project(Victory)
set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE sources
    src/*.cpp
)

find_package(Vulkan REQUIRED)

add_subdirectory(externs/glfw)
add_subdirectory(externs/glm)

set(IMGUI_FILES
    externs/imgui/imgui.cpp
    externs/imgui/imgui_demo.cpp
    externs/imgui/imgui_draw.cpp
    externs/imgui/imgui_tables.cpp
    externs/imgui/imgui_widgets.cpp
    externs/imgui/backends/imgui_impl_glfw.cpp
    externs/imgui/backends/imgui_impl_vulkan.cpp
    )

add_library(Victory ${sources} ${IMGUI_FILES})
add_dependencies(Victory Shaders)

target_include_directories(Victory PRIVATE externs/stb_image externs/tiny_obj_loader externs/imgui)
target_link_libraries(Victory PRIVATE Vulkan::Vulkan glfw glm)

set_target_properties(Victory PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

if(UNIX)
target_compile_options(Victory PRIVATE
    -Wall -Wextra -Wpedantic -Werror 
)
endif()

cmake_minimum_required(VERSION 3.0.0)

set(SHADER_DIR ${CMAKE_SOURCE_DIR}/Victory/shaders)
file(GLOB_RECURSE GLSL_SOURCE_FILES
    ${SHADER_DIR}/*.vert
    ${SHADER_DIR}/*.frag
)

foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
if(WIN32)
    set(SPIRV ${CMAKE_BINARY_DIR}/VictoryTest/$<CONFIG>/${FILE_NAME}.spv)
else()
    set(SPIRV ${CMAKE_BINARY_DIR}/VictoryTest/${FILE_NAME}.spv)
endif()
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND glslc ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL}
    )
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders
    DEPENDS ${SPIRV_BINARY_FILES}
)

project(VictoryTest)
set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE sources
    *.cpp
)

find_package(Vulkan REQUIRED)

set(IMGUI_FILES
    ../Victory/externs/imgui/imgui.cpp
    ../Victory/externs/imgui/imgui_demo.cpp
    ../Victory/externs/imgui/imgui_draw.cpp
    ../Victory/externs/imgui/imgui_tables.cpp
    ../Victory/externs/imgui/imgui_widgets.cpp
    ../Victory/externs/imgui/backends/imgui_impl_glfw.cpp
    ../Victory/externs/imgui/backends/imgui_impl_vulkan.cpp
    )

add_executable(VictoryTest ${sources} ${IMGUI_FILES})
add_dependencies(VictoryTest Shaders)

target_include_directories(VictoryTest PRIVATE ../Victory/externs/stb_image ../Victory/externs/tiny_obj_loader ../Victory/externs/imgui)
target_link_libraries(VictoryTest PRIVATE Vulkan::Vulkan glfw glm)

if(UNIX)
target_compile_options(VictoryTest PRIVATE
    -Wall -Wextra -Wpedantic -Werror 
)
endif()

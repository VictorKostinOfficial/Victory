cmake_minimum_required(VERSION 3.0.0)

set(SHADER_DIR ${CMAKE_SOURCE_DIR}/Victory/shaders)
file(GLOB_RECURSE GLSL_SOURCE_FILES
    ${SHADER_DIR}/*.vert
    ${SHADER_DIR}/*.frag
)

foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV ${CMAKE_SOURCE_DIR}/Victory/shaders/${FILE_NAME}.spv)
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

project(Victory)
set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE sources
    src/*.cpp
)

find_package(Vulkan REQUIRED)

add_subdirectory(externs/glfw)
add_subdirectory(externs/imgui)

add_library(Victory ${sources})

add_dependencies(Victory Shaders)

target_link_libraries(Victory PRIVATE Vulkan::Vulkan glfw imgui)

target_compile_options(Victory PRIVATE
  -Wall -Wextra -Wpedantic #-Werror 
)

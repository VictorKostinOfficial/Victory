cmake_minimum_required(VERSION 3.0.0)

set(SHADER_DIR ${CMAKE_SOURCE_DIR}/shaders)
file(GLOB_RECURSE GLSL_SOURCE_FILES
    ${SHADER_DIR}/*.vert
    ${SHADER_DIR}/*.frag
)

foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV ${CMAKE_SOURCE_DIR}/bin/$<CONFIG>/shaders/${FILE_NAME}.spv)
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

find_package(Vulkan REQUIRED)

add_subdirectory(extern/glfw)
add_subdirectory(extern/imgui)

file(GLOB_RECURSE sources
    src/*.cpp
    src/renderer/*.cpp
    src/renderer/vulkan_renderer/*.cpp
)

add_library(Victory ${sources})

set_target_properties(Victory
    PROPERTIES
        CXX_STANDARD 20
)

add_dependencies(Victory Shaders)

target_link_libraries(Victory PRIVATE Vulkan::Vulkan glfw imgui)
target_include_directories(Victory PRIVATE src extern/imgui extern/imgui/backends)

if(WIN32)
    target_link_libraries(Victory PRIVATE dwmapi.lib)
    target_compile_definitions(Victory PRIVATE VK_USE_PLATFORM_WIN32_KHR)
endif()

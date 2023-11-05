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

project(VulkanTriangle)

find_package(Vulkan REQUIRED)
add_subdirectory(extern/glfw)

file(GLOB_RECURSE Headers
  src/*.h
)
file(GLOB_RECURSE Sources
  src/*.cpp
)

add_executable(VulkanTriangle ${Headers} ${Sources})
set_target_properties(VulkanTriangle
    PROPERTIES
        CXX_STANDARD 20
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)

add_dependencies(VulkanTriangle Shaders)

if(WIN32)
    target_link_libraries(VulkanTriangle PRIVATE dwmapi.lib)
    target_compile_definitions(VulkanTriangle PRIVATE VK_USE_PLATFORM_WIN32_KHR)
endif()

target_link_libraries(VulkanTriangle PRIVATE Vulkan::Vulkan glfw)

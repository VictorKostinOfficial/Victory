cmake_minimum_required(VERSION 3.0.0)

project(VulkanTriangle)

find_package(Vulkan REQUIRED)
add_subdirectory(extern/glfw)


file(GLOB_RECURSE Headers
  "src/*.h"
)
file(GLOB_RECURSE Sources
  "src/*.cpp"
)


add_executable(VulkanTriangle ${Headers} ${Sources})

if(WIN32)
    target_link_libraries(VulkanTriangle PRIVATE dwmapi.lib)
    target_compile_definitions(VulkanTriangle PRIVATE VK_USE_PLATFORM_WIN32_KHR)
endif()

target_link_libraries(VulkanTriangle PRIVATE Vulkan::Vulkan glfw)

set_property(TARGET VulkanTriangle
    PROPERTY
    CXX_STANDARD 20
)
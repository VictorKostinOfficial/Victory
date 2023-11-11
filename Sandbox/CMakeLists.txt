cmake_minimum_required(VERSION 3.0.0)
project(Sandbox)
set(CMAKE_CXX_STANDARD 23)

find_package(Vulkan REQUIRED)
add_executable(Sandbox src/Sandbox.cpp)

target_include_directories(Sandbox PRIVATE ../Victory/src)
target_link_libraries(Sandbox PRIVATE Victory Vulkan::Vulkan)
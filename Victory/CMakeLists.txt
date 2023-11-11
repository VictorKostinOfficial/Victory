cmake_minimum_required(VERSION 3.0.0)
project(Victory)
set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE sources
    src/*.cpp
)

find_package(Vulkan REQUIRED)

add_library(Victory ${sources})
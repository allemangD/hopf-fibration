cmake_minimum_required(VERSION 3.10)
project(hopf)

set(CMAKE_CXX_STANDARD 17)

add_library(glad vendor/glad/src/glad.c)
target_include_directories(glad PUBLIC vendor/glad/include)

option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)
add_subdirectory(vendor/glfw)

add_subdirectory(main)

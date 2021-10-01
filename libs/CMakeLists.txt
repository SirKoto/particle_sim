cmake_minimum_required(VERSION 3.10)

add_subdirectory(glad)
add_subdirectory(imgui)
add_subdirectory(glm)

# tinyply
add_library(tinyply STATIC tinyply/tinyply.cpp tinyply/tinyply.h)
target_include_directories(tinyply PUBLIC tinyply/)
target_compile_features(tinyply PRIVATE cxx_std_11)


set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
add_subdirectory(glfw)


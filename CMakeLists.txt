cmake_minimum_required(VERSION 3.0.0)

# Declare project (qq)
project(qq VERSION 0.0.1 LANGUAGES C)

# Binary output path
set(EXECUTABLE_OUTPUT_PATH "output/bin")

# Allow debug symbols
set(CMAKE_BUILD_TYPE Debug)

# Specify executable
add_executable(qq "src/main.c")

# Add header include directory
target_include_directories(qq PUBLIC "src/public")

# Link glibc
target_link_libraries(qq -static-libgcc)

# Vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(qq Vulkan::Vulkan)

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory("./dependencies/glfw")
target_link_libraries(qq glfw)

# CGLM
add_subdirectory("./dependencies/cglm")
target_link_libraries(qq cglm)

# Add STB include directories
target_include_directories(qq PUBLIC "./dependencies/stb")


cmake_minimum_required(VERSION 3.0.0)

# Declare project
project(qq VERSION 0.0.1 LANGUAGES C)

# Binary output path
set(EXECUTABLE_OUTPUT_PATH "output/bin")

# Specify executable
add_executable(qq "src/main.c")


cmake_minimum_required(VERSION 3.16)
project(flecs-systems-webgpu)

set(CMAKE_C_STANDARD 99)

# Add source files
set(SOURCES
    src/main.c
    src/geometry/geometry.c
    src/resources/resource_manager.c
    src/rendering/render_system.c
    src/math/math_utils.c
    src/shaders/shader_sources.c
)

# Create library
add_library(flecs_systems_webgpu ${SOURCES})

# Include directories
target_include_directories(flecs_systems_webgpu PUBLIC 
    include
    src
)

# For now, let's just test compilation without WebGPU headers
# This will help us catch syntax errors in our C code
target_compile_definitions(flecs_systems_webgpu PRIVATE
    FLECS_SYSTEMS_WEBGPU_STANDALONE_BUILD=1
)

# Add compiler flags for better error reporting
target_compile_options(flecs_systems_webgpu PRIVATE 
    -Wall -Wextra -Werror=implicit-function-declaration
)

# Test executable for basic compilation
add_executable(test_compile test/test_compile.c)
target_link_libraries(test_compile flecs_systems_webgpu)
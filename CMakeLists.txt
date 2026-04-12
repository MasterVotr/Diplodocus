cmake_minimum_required(VERSION 3.28)
project(
    diplodocus
    VERSION 0.1.0
    LANGUAGES CXX C CUDA
)

# Language standards
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CUDA_STANDARD 20)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_EXTENSIONS OFF)

# Avoids static cudart-driven nvlink noise on Linux
set(CMAKE_CUDA_RUNTIME_LIBRARY Shared)

# Keep compile database for tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies
include(Dependencies.cmake)

# -----------------------------
# Source split
# -----------------------------
file(GLOB_RECURSE APP_SOURCE_FILES CONFIGURE_DEPENDS
    "src/*.cpp"
    "src/*.cc"
)

# Do not compile GPU host wrappers twice via the app target
list(FILTER APP_SOURCE_FILES EXCLUDE REGEX ".*/src/gpu/.*")

file(GLOB_RECURSE GPU_CPP_FILES CONFIGURE_DEPENDS
    "src/gpu/*.cc"
)
file(GLOB_RECURSE GPU_CUDA_FILES CONFIGURE_DEPENDS
    "src/gpu/*.cu"
)

# -----------------------------
# Dedicated CUDA library
# -----------------------------
add_library(diplodocus_gpu STATIC
    ${GPU_CPP_FILES}
    ${GPU_CUDA_FILES}
)

set_target_properties(diplodocus_gpu PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_RESOLVE_DEVICE_SYMBOLS ON
    CUDA_ARCHITECTURES 86
    CUDA_RUNTIME_LIBRARY Shared
)

target_include_directories(diplodocus_gpu
    PUBLIC
    src
)

target_compile_options(diplodocus_gpu PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -pedantic>
    $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-Wall,-Wextra,-Wpedantic>
    $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-Wno-gnu-line-marker>
)

# -----------------------------
# Host executable
# -----------------------------
add_executable(${PROJECT_NAME}
    ${APP_SOURCE_FILES}
)

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    GLFW_INCLUDE_NONE
)

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra -pedantic
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    diplodocus_gpu
    glfw
    imgui
    glad
    OpenGL::GL
    tinyobjloader
    json
    tomlplusplus
)

option(ENABLE_DEBUG "Enable asan and deugging" OFF)

if(ENABLE_DEBUG)
    target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address -g)
    target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address -g)
endif()

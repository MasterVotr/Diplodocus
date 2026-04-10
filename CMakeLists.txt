cmake_minimum_required(VERSION 3.28)
project(
    guiapp
    VERSION 0.1.0
    LANGUAGES CXX C CUDA
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies
include(Dependencies.cmake)

# Application
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "src/*.cpp" "src/*.cc")
file(GLOB_RECURSE CUDA_SOURCE_FILES CONFIGURE_DEPENDS "src/*.cu")
add_executable(${PROJECT_NAME} "${SOURCE_FILES}" "${CUDA_SOURCE_FILES}")

add_compile_definitions(
    GLFW_INCLUDE_NONE
)

# Link everything
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    imgui
    glad
    OpenGL::GL
    tinyobjloader
    json
    tomlplusplus
)

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)

set(ENABLE_DEBUG "Enable asan and deugging" OFF)

if (ENABLE_DEBUG)
    target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address -g)
    target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address -g)
endif()

# End of file

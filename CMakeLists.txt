cmake_minimum_required(VERSION 3.28)
project(
	guiapp
	VERSION 0.1.0
	LANGUAGES CXX C
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# OpenGL
find_package(OpenGL REQUIRED)

# GLFW
add_subdirectory(third-party/glfw)

# GLAD
set(GLAD_DIR third-party/glad)
add_library(glad STATIC ${GLAD_DIR}/src/glad.c)
target_include_directories(glad PUBLIC ${GLAD_DIR}/include)
target_link_libraries(glad PUBLIC OpenGL::GL)

# ImGui
set(IMGUI_DIR third-party/imgui)
add_library(imgui STATIC
	${IMGUI_DIR}/imgui.cpp
	${IMGUI_DIR}/imgui_draw.cpp
	${IMGUI_DIR}/imgui_tables.cpp
	${IMGUI_DIR}/imgui_widgets.cpp
	${IMGUI_DIR}/imgui_demo.cpp
	${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
	${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
	${IMGUI_DIR}
	${IMGUI_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

# Application
file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.cc")
add_executable(${PROJECT_NAME} "${SOURCE_FILES}")

# Link everything
target_link_libraries(${PROJECT_NAME} PRIVATE
	glfw
	imgui
	glad
	OpenGL::GL
)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)

set(ENABLE_DEBUG "Enable asan and deugging" OFF)

if (ENABLE_DEBUG)
	target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address -g)
	target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address -g)
endif()

# End of file

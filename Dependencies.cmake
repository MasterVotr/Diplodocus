###############
# DEPENDENCIES #
###############

# OpenGL
find_package(OpenGL REQUIRED)

# GLFW
add_subdirectory(third-party/glfw)

# GLAD
set(GLAD_DIR third-party/glad)
add_library(glad STATIC ${GLAD_DIR}/src/gl.c)
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

# rapidobj
add_library(rapidobj INTERFACE)
target_include_directories(rapidobj INTERFACE third-party/rapidobj)

# json
add_library(json INTERFACE)
target_include_directories(json INTERFACE third-party/json)

# tomlplusplus
add_library(tomlplusplus INTERFACE)
target_include_directories(tomlplusplus INTERFACE third-party/tomlplusplus)


// C++ STD headers
#include <cstddef>
#include <iostream>

// Third-party headers
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>
#include <imgui_internal.h>

// Window size
const unsigned int WINDOW_WIDTH = 1920;
const unsigned int WINDOW_HEIGHT = 1080;

// Error callback for GLFW
void glfw_error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

int main() {
    // -----------------------------
    // Initialize GLFW
    // -----------------------------
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // OpenGL 4.1 Core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
    GLFWwindow* window = glfwCreateWindow((int)WINDOW_WIDTH * main_scale, (int)WINDOW_HEIGHT * main_scale,
                                          "ImGui Docking Test", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  // Enable vsync

    // -----------------------------
    // Initialize GLAD
    // -----------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    // -----------------------------
    // Initialize ImGui
    // -----------------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;    // Enable multi-viewport

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 410");  // GLSL 410 for OpenGL 4.1

    // Our state
    bool show_demo_window = true;

    // -----------------------------
    // Main loop
    // -----------------------------
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Create a dockspace in main viewport
        ImGuiID dockspace_id = ImGui::GetID("Dockspace");
        ImGuiViewport* main_viewport = ImGui::GetMainViewport();

        // Setup dockspace
        if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr) {
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, main_viewport->Size);
            ImGuiID dock_id_main = dockspace_id;
            ImGuiID dock_id_left = 0;
            ImGuiID dock_id_right = 0;
            ImGuiID dock_id_bottom = 0;
            ImGui::DockBuilderSplitNode(dock_id_main, ImGuiDir_Left, 0.166f, &dock_id_left, &dock_id_main);
            ImGui::DockBuilderSplitNode(dock_id_main, ImGuiDir_Right, 0.2f, &dock_id_right, &dock_id_main);
            ImGui::DockBuilderSplitNode(dock_id_main, ImGuiDir_Down, 0.333f, &dock_id_bottom, &dock_id_main);
            ImGui::DockBuilderDockWindow("MainPanel", dock_id_main);
            ImGui::DockBuilderDockWindow("LeftPanel", dock_id_left);
            ImGui::DockBuilderDockWindow("RightPanel", dock_id_right);
            ImGui::DockBuilderDockWindow("BottomPanel", dock_id_bottom);
            ImGui::DockBuilderFinish(dockspace_id);
        }

        // Submit dockspace
        ImGui::DockSpaceOverViewport(dockspace_id, main_viewport, ImGuiDockNodeFlags_PassthruCentralNode);

	// Setup windows
        ImGui::Begin("MainPanel");
        ImGui::Text("width: %.1f, height: %.1f", ImGui::GetWindowSize().x, ImGui::GetWindowSize().y);
        ImGui::End();

        ImGui::Begin("LeftPanel");
        ImGui::Text("width: %.1f, height: %.1f", ImGui::GetWindowSize().x, ImGui::GetWindowSize().y);
        ImGui::End();

        ImGui::Begin("RightPanel");
        ImGui::Text("width: %.1f, height: %.1f", ImGui::GetWindowSize().x, ImGui::GetWindowSize().y);
        ImGui::End();

        ImGui::Begin("BottomPanel");
        ImGui::Text("width: %.1f, height: %.1f", ImGui::GetWindowSize().x, ImGui::GetWindowSize().y);
        ImGui::End();

        if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and render additional platform windows (for multi-viewport)
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
    }

    // -----------------------------
    // Cleanup
    // -----------------------------
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

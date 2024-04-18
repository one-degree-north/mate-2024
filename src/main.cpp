
#include <iostream>
#include <filesystem>
#include <sstream>
#include <fstream>

#include "nfd.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "imgui_internal.h"
#include "implot.h"

#include <GLFW/glfw3.h>

#include "font.h"

#include "pi.h"
#include "camera_stream.h"
#include "photogrammetry.h"
#include "controls.h"

bool interrupted = false;
void interrupt(int _signal) { interrupted = true; }

std::string get_server_address() {
    if (const char* server_address_env = getenv("SERVER_ADDRESS"))
        return server_address_env;

    std::string server_address;
    std::cout << "Enter Server Address: ";
    std::cin >> server_address;

    return server_address;
}

void initialiseLibraries() {
    if (!glfwInit()) { // GLFW (for windowing)
        std::cerr << "Failed to initialise GLFW" << std::endl;
        exit(1);
    }

    NFD_Init(); // Native File Dialog (for file selection)
}

GLFWwindow* initialiseGui() {
    // OpenGL 3.2
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // Create Window
    GLFWwindow* window = glfwCreateWindow(1280, 720, "MATE Client", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Failed to create window" << std::endl;
        exit(1);
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Initialise ImGui (for GUI) with OpenGL + GLFW backend
    ImGui::CreateContext();
    ImPlot::CreateContext(); // ImPlot (for graphing)
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    ImGuiIO &io = ImGui::GetIO(); (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Setup Font (JetBrains Mono Nerd Font)
    ImFontConfig config;
    config.OversampleH = 3;
    config.OversampleV = 3;
    io.FontGlobalScale = 0.5f; // scale is half, and font size is 2x to account for retina displays
    io.Fonts->AddFontFromMemoryCompressedTTF(font_compressed_data, font_compressed_size, 28.0f, &config);

    // Default Styling
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 4;
    style.FrameRounding = 2;
    style.GrabRounding = 2;
    style.TabRounding = 4;
    style.FramePadding.x = 6;
    style.WindowTitleAlign.x = 0.5;

    return window;
}

int main(int argc, char** argv) {
    initialiseLibraries();
    GLFWwindow* window = initialiseGui();

    std::string server_address = get_server_address();

    Pi pi(server_address);
    Controls controls(pi);
    CameraStream cameraStream(pi, server_address);
    Photogrammetry photogrammetry(cameraStream);


    while (!glfwWindowShouldClose(window) || interrupted) {
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) break;


        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

        cameraStream.ShowCameraStream();
        photogrammetry.ShowPhotogrammetryWindow();
        controls.ShowControlsWindow();

        ImGui::Render();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
}

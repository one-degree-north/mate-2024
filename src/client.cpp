#include <iostream>

#include <gst/gst.h>
#include <gst/app/app.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "comms.h"

#include <GLFW/glfw3.h>

GstElement* pipeline;
void interrupt(int _signal) {
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    }

    exit(0);
}

static void glfw_error_callback (int error, const char *description) {
    g_print ("Glfw Error %d: %s\n", error, description);
}

std::string get_server_address() {
    if (const char* server_address_env = getenv("SERVER_ADDRESS"))
        return server_address_env;

    std::string server_address;
    std::cout << "Enter Server Address: ";
    std::cin >> server_address;

    return server_address;
}

int main(int argc, char** argv) {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return 1;


    #if defined(__APPLE__)
        const char* glsl_version = "#version 150";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #else
        const char* glsl_version = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    GLFWwindow* window = glfwCreateWindow(1280, 720, "MATE Client", nullptr, nullptr);
    if (window == nullptr) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );

    gst_init(&argc, &argv);
    pipeline = gst_parse_launch(R"(udpsrc port=6970 caps="application/x-rtp, encoding-name=JPEG" ! rtpjpegdepay ! jpegdec ! videoconvert ! appsink name=sink caps="video/x-raw, format=RGB")", nullptr);

    GstElement* sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    if (!sink) {
        std::cerr << "Failed to get sink from pipeline" << std::endl;
        return 1;
    }

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    signal(SIGINT, interrupt);
    signal(SIGTERM, interrupt);

    int width = 0;
    int height = 0;

    GLuint videotex;
    glGenTextures (1, &videotex);
    glBindTexture (GL_TEXTURE_2D, videotex);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    Communication communication(get_server_address(), 7070);

    bool pingWaiting = true;
    auto pingStart = std::chrono::high_resolution_clock::now();
    auto pingDuration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - pingStart);

    struct vec3 { double x, y, z; };
    struct vec4 { double w, x, y, z; };
    struct {
        vec3 acc, mag, gyro, euler, lin_acc, grav;
        double temp;
        vec4 quat;
    } sensor_data {};
    struct {
        int system, gyro, acc, mag;
    } calibration_status {};
    bool bnoDataReceived = false;
    bool bnoIsConfiguring = false;

    std::vector<uint8_t> buffer;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) break;

        GstSample* videosample = gst_app_sink_try_pull_sample(GST_APP_SINK(sink), 5 * GST_MSECOND);
        if (videosample) {
            if (width == 0 || height == 0) {
                GstCaps *caps = gst_sample_get_caps(videosample);
                GstStructure *structure = gst_caps_get_structure(caps, 0);

                gst_structure_get_int(structure, "width", &width);
                gst_structure_get_int(structure, "height", &height);
            }

            GstBuffer* videobuf = gst_sample_get_buffer(videosample);
            GstMapInfo map;

            gst_buffer_map (videobuf, &map, GST_MAP_READ);

            glBindTexture(GL_TEXTURE_2D, videotex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, map.data);
            gst_buffer_unmap(videobuf, &map);
            gst_sample_unref(videosample);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::DockSpaceOverViewport();

        ImGui::ShowMetricsWindow();

        ImGui::ShowDemoWindow();

        ImGui::Begin("Ping");
        if (ImGui::Button("Ping")) {
            communication.send({0x01});
            pingWaiting = true;
            pingStart = std::chrono::high_resolution_clock::now();
        }

        if (!pingWaiting) {
            ImGui::Text("Ping Duration: %lld ms", pingDuration.count());
        } else {
            ImGui::Text("Ping Duration: Waiting");
        }
        ImGui::End();

        ImGui::Begin("Sensor Data");
        ImGui::Text("Sensor Mode: %s", !bnoDataReceived ? "Waiting For Packet" : bnoIsConfiguring ? "Calibration" : "Sensor Data");

        if (bnoDataReceived) {
            if (bnoIsConfiguring) {
                ImGui::SameLine();
                if (ImGui::Button("Switch To Sensor Data")) communication.send({0x03, 0x01});

                ImGui::Text("Calibration Status ->");
                ImGui::Text("System: %d", calibration_status.system);
                ImGui::Text("Gyroscope: %d", calibration_status.gyro);
                ImGui::Text("Accelerometer: %d", calibration_status.acc);
                ImGui::Text("Magnetometer: %d", calibration_status.mag);
            } else {
                ImGui::SameLine();
                if (ImGui::Button("Switch To Calibration")) communication.send({0x03, 0x00});

                ImGui::Text("Sensor Data ->");
                ImGui::Text("Acceleration (m/s): (x = %0.2f, y = %0.2f, z = %0.2f)", sensor_data.acc.x, sensor_data.acc.y, sensor_data.acc.z);
                ImGui::Text("Magnetometer (uT): (x = %0.2f, y = %0.2f, z = %0.2f)", sensor_data.mag.x, sensor_data.mag.y, sensor_data.mag.z);
                ImGui::Text("Gyroscope (dps): (x = %0.2f, y = %0.2f, z = %0.2f)", sensor_data.mag.x, sensor_data.mag.y, sensor_data.mag.z);
                ImGui::Text("Euler Angles (degrees): (x = %0.2f, y = %0.2f, z = %0.2f)", sensor_data.euler.x, sensor_data.euler.y, sensor_data.euler.z);
                ImGui::Text("Quaternion: (w = %0.2f, x = %0.2f, y = %0.2f, z = %0.2f)", sensor_data.quat.w, sensor_data.quat.x, sensor_data.quat.y, sensor_data.quat.z);
                ImGui::Text("Linear Acceleration (m/s): (x = %0.2f, y = %0.2f, z = %0.2f)", sensor_data.lin_acc.x, sensor_data.lin_acc.y, sensor_data.lin_acc.z);
                ImGui::Text("Gravity (m/s): (x = %0.2f, y = %0.2f, z = %0.2f)", sensor_data.grav.x, sensor_data.grav.y, sensor_data.grav.z);
                ImGui::Text("Temperature (C): %0.2f", sensor_data.temp);
            }
        }



        ImGui::End();

        if (communication.recv_nonblocking(buffer)) {
            switch (buffer[0]) {
                case 0x02:
                    pingWaiting = false;
                    pingDuration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - pingStart);
                    break;
                case 0x03:
                    bnoDataReceived = true;
                    bnoIsConfiguring = true;

                    calibration_status.system = (buffer[1] >> 6) & 0b11;
                    calibration_status.gyro = (buffer[1] >> 4) & 0b11;
                    calibration_status.acc = (buffer[1] >> 2) & 0b11;
                    calibration_status.mag = buffer[1] & 0b11;
                    break;
                case 0x04:
                    bnoDataReceived = true;
                    bnoIsConfiguring = false;

                    sensor_data.acc.x = (int16_t) ((buffer[2] << 8 | buffer[1]) & 0xFFFF) / 100.0;
                    sensor_data.acc.y = (int16_t) ((buffer[4] << 8 | buffer[3]) & 0xFFFF) / 100.0;
                    sensor_data.acc.z = (int16_t) ((buffer[6] << 8 | buffer[5]) & 0xFFFF) / 100.0;

                    sensor_data.mag.x = (int16_t) ((buffer[8] << 8 | buffer[7]) & 0xFFFF) / 16.0;
                    sensor_data.mag.y = (int16_t) ((buffer[10] << 8 | buffer[9]) & 0xFFFF) / 16.0;
                    sensor_data.mag.z = (int16_t) ((buffer[12] << 8 | buffer[11]) & 0xFFFF) / 16.0;

                    sensor_data.gyro.x = (int16_t) ((buffer[14] << 8 | buffer[13]) & 0xFFFF) / 16.0;
                    sensor_data.gyro.y = (int16_t) ((buffer[16] << 8 | buffer[15]) & 0xFFFF) / 16.0;
                    sensor_data.gyro.z = (int16_t) ((buffer[18] << 8 | buffer[17]) & 0xFFFF) / 16.0;

                    sensor_data.euler.x = (int16_t) ((buffer[20] << 8 | buffer[19]) & 0xFFFF) / 16.0;
                    sensor_data.euler.y = (int16_t) ((buffer[22] << 8 | buffer[21]) & 0xFFFF) / 16.0;
                    sensor_data.euler.z = (int16_t) ((buffer[24] << 8 | buffer[23]) & 0xFFFF) / 16.0;

                    sensor_data.quat.w = (int16_t) ((buffer[26] << 8 | buffer[25]) & 0xFFFF) / 16384.0;
                    sensor_data.quat.x = (int16_t) ((buffer[28] << 8 | buffer[27]) & 0xFFFF) / 16384.0;
                    sensor_data.quat.y = (int16_t) ((buffer[30] << 8 | buffer[29]) & 0xFFFF) / 16384.0;
                    sensor_data.quat.z = (int16_t) ((buffer[32] << 8 | buffer[31]) & 0xFFFF) / 16384.0;

                    sensor_data.lin_acc.x = (int16_t) ((buffer[34] << 8 | buffer[33]) & 0xFFFF) / 100.0;
                    sensor_data.lin_acc.y = (int16_t) ((buffer[36] << 8 | buffer[35]) & 0xFFFF) / 100.0;
                    sensor_data.lin_acc.z = (int16_t) ((buffer[38] << 8 | buffer[37]) & 0xFFFF) / 100.0;

                    sensor_data.grav.x = (int16_t) ((buffer[40] << 8 | buffer[39]) & 0xFFFF) / 100.0;
                    sensor_data.grav.y = (int16_t) ((buffer[42] << 8 | buffer[41]) & 0xFFFF) / 100.0;
                    sensor_data.grav.z = (int16_t) ((buffer[44] << 8 | buffer[43]) & 0xFFFF) / 100.0;

                    sensor_data.temp = buffer[45];
                    break;
            }
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Camera Stream");
        ImVec2 windowSize = ImGui::GetWindowSize();
        windowSize.y -= ImGui::GetFrameHeight();

        ImVec2 imageSize((float) width, (float) height);

        if (windowSize.x / windowSize.y > imageSize.x / imageSize.y) {
            imageSize.x = windowSize.y * imageSize.x / imageSize.y;
            imageSize.y = windowSize.y;
            ImGui::SetCursorPosX((windowSize.x - imageSize.x) / 2);
        } else {
            imageSize.y = windowSize.x * imageSize.y / imageSize.x;
            imageSize.x = windowSize.x;
            ImGui::SetCursorPosY(ImGui::GetFrameHeight() + (windowSize.y - imageSize.y) / 2);
        }

        ImGui::Image((void*)(intptr_t)videotex, imageSize, ImVec2(0, 0), ImVec2(1, 1));
        ImGui::End();
        ImGui::PopStyleVar();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
}

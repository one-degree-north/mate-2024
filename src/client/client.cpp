#include <iostream>
#include <filesystem>
#include <sstream>
#include <fstream>

#include "nfd.hpp"

#include <gst/gst.h>
#include <gst/app/app.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "comms.h"
#include "font.h"
#include "imgui_internal.h"
#include "implot.h"

#include <GLFW/glfw3.h>

extern "C" {
    bool is_completed();
    double get_progress();
    double get_eta();

    void run_photogrammetry_session(const char* path);
}

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

    NFD::Guard nfdGuard;

    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "MATE Client", nullptr, nullptr);
    if (window == nullptr) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImFontConfig config;
    config.OversampleH = 3;
    config.OversampleV = 3;
    io.FontGlobalScale = 0.5f;
    io.Fonts->AddFontFromMemoryCompressedTTF(font_compressed_data, font_compressed_size, 28.0f, &config);

    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 4;
    style.FrameRounding = 2;
    style.GrabRounding = 2;
    style.TabRounding = 4;
    style.FramePadding.x = 6;
    style.WindowTitleAlign.x = 0.5;


    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    ImPlot::CreateContext();

    gst_init(&argc, &argv);
    GError *err = nullptr;
    if (!std::filesystem::exists("images")) std::filesystem::create_directory("images");
    static GstElement* pipeline = gst_parse_launch(
    "udpsrc port=6970 ! application/x-rtp,encoding-name=JPEG ! rtpjpegdepay ! jpegparse ! jpegdec ! tee name=t "
        "! queue leaky=downstream ! videoconvert ! video/x-raw,format=RGB ! appsink name=gui-sink drop=true sync=false "
        "t. ! queue leaky=downstream ! valve name=image-valve drop=true ! videocrop name=image-crop bottom=0 ! videorate skip-to-first=true max-closing-segment-duplication-duration=0 ! capsfilter caps-change-mode=immediate name=image-rate-filter caps=video/x-raw,framerate=1/1 ! jpegenc ! multifilesink name=image-sink location=images/image%d.jpeg async=false",
    &err);
    if (err) {
        std::cerr << "Failed to create pipeline: " << err->message << std::endl;
        return 1;
    }

    int imageFrameRate = 1;
    bool imageRecording = false;

//    GstElement* tee = gst_bin_get_by_name(GST_BIN(pipeline), "t");
    GstElement* imageValve = gst_bin_get_by_name(GST_BIN(pipeline), "image-valve");
    GstElement* imageRateFilter = gst_bin_get_by_name(GST_BIN(pipeline), "image-rate-filter");
    GstElement* imageCrop = gst_bin_get_by_name(GST_BIN(pipeline), "image-crop");
    GstElement* imageSink = gst_bin_get_by_name(GST_BIN(pipeline), "image-sink");

    GstElement* sink = gst_bin_get_by_name(GST_BIN(pipeline), "gui-sink");
    if (!sink) {
        std::cerr << "Failed to get sink from pipeline" << std::endl;
        return 1;
    }

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
//    gst_element_unlink(tee, imageQueue);
//    gst_element_set_state(imageQueue, GST_STATE_NULL);
    signal(SIGINT, interrupt);
    signal(SIGTERM, interrupt);

    int videoWidth = 0;
    int videoHeight = 0;
    GLuint videoTexture;

    glGenTextures (1, &videoTexture);
    glBindTexture (GL_TEXTURE_2D, videoTexture);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    Communication communication(get_server_address(), 7070);

    bool pingWaiting = true;
    auto pingStart = std::chrono::high_resolution_clock::now();
    auto pingDuration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - pingStart);

    struct vec3 { double x, y, z; };
    struct vec4 { double w, x, y, z; };
    struct { vec3 acc, mag, gyro, euler, lin_acc, grav; double temp; vec4 quat; } sensor_data {};
    struct { int system, gyro, acc, mag; } calibration_status {};
    bool bnoDataReceived = false;
    bool bnoIsConfiguring = false;

    std::vector<std::pair<double, typeof sensor_data>> sensor_history;

    union {
        struct { float front, side, up, pitch, roll, yaw, speed; };
        uint8_t data[24] {};
    } thrusterData {};
    float claw0 = 0, claw1 = 0;

    std::vector<uint8_t> buffer;

    bool shouldUseController = false;
    float xAxis = 0;
    float yAxis = 0;


    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) break;

        GstSample* videoSample = gst_app_sink_try_pull_sample(GST_APP_SINK(sink),  GST_NSECOND);
        if (videoSample) {
            if (videoWidth == 0 || videoHeight == 0) {
                GstCaps *caps = gst_sample_get_caps(videoSample);
                GstStructure *structure = gst_caps_get_structure(caps, 0);

                gst_structure_get_int(structure, "width", &videoWidth);
                gst_structure_get_int(structure, "height", &videoHeight);
            }

            GstBuffer* videoBuffer = gst_sample_get_buffer(videoSample);
            GstMapInfo map;

            gst_buffer_map (videoBuffer, &map, GST_MAP_READ);

            glBindTexture(GL_TEXTURE_2D, videoTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, videoWidth, videoHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, map.data);
            gst_buffer_unmap(videoBuffer, &map);
            gst_sample_unref(videoSample);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags host_window_flags = 0;
        host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
        host_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace", NULL, host_window_flags);
        ImGui::PopStyleVar(3);

        ImGuiID dockspace_id = ImGui::GetID("DockSpace");
        if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr) {
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

            ImGuiID dock_main_id = dockspace_id;
            ImGuiID camera_space = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.6f, nullptr, &dock_main_id);
            ImGuiID thruster_space = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.35f, nullptr, &dock_main_id);
            ImGuiID reconstruction_space = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.35f, nullptr, &dock_main_id);

            ImGui::DockBuilderDockWindow("Camera Stream", camera_space);
            ImGui::DockBuilderDockWindow("Thruster Control", thruster_space);
            ImGui::DockBuilderDockWindow("Reconstruction", reconstruction_space);
            ImGui::DockBuilderDockWindow("Sensor Data", dock_main_id);


            ImGui::DockBuilderFinish(dockspace_id);
        }

        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), 0, nullptr);
        ImGui::End();


        ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);
        ImGui::SetNextWindowPos(ImVec2(100, 500), ImGuiCond_Once);
        ImGui::ShowMetricsWindow();

        ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);
        ImGui::ShowDemoWindow();

        ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(100, 560), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(150, 70), ImGuiCond_FirstUseEver);
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

                static enum { ACCELERATION, MAGNETOMETER, GYROSCOPE, EULER, QUATERNION, LIN_ACC, GRAVITY, TEMPERATURE} graphType = ACCELERATION;

                static const struct {
                    std::string name;
                    uint8_t offset;
                    uint8_t values;
                    double min = -10;
                    double max = 10;
                    std::vector<std::string> labels = {"X", "Y", "Z", "W"};
                } graphTypes[] = {
                    {"Acceleration", 0, 3},
                    {"Magnetometer", 3, 3, -50, 50},
                    {"Gyroscope", 6, 3, -50, 50},
                    {"Euler Angles", 9, 3, -360, 360},
                    {"Quaternion", 12, 4, -1, 1},
                    {"Linear Acceleration", 16, 3},
                    {"Gravity", 19, 3},
                    {"Temperature", 22, 1, -10, 10, {"Temp"}}
                };
                if (ImGui::BeginCombo("Data Type", graphTypes[graphType].name.c_str())) {
                    for (int i = 0; i < IM_ARRAYSIZE(graphTypes); i++) {
                        bool isSelected = graphType == i;
                        if (ImGui::Selectable(graphTypes[i].name.c_str(), isSelected)) graphType = (typeof graphType) i;
                        if (isSelected) ImGui::SetItemDefaultFocus();
                    }

                    ImGui::EndCombo();
                }

                if (ImPlot::BeginPlot("##", ImVec2(0, 0), ImPlotFlags_NoFrame)) {
                    double t = ImGui::GetTime();
                    ImPlot::SetupAxesLimits(std::max(t - 10, 0.0), std::max(t, 10.0), graphTypes[graphType].min, graphTypes[graphType].max, ImGuiCond_Always);


                    if (!sensor_history.empty()) {
                        for (int i = 0; i < graphTypes[graphType].values; i++) {
                            ImPlot::PlotLine(graphTypes[graphType].labels[i].c_str(), &sensor_history[0].first, &sensor_history[0].second.acc.x + i + 8 * graphTypes[graphType].offset,
                                             sensor_history.size(), 0, 0, sizeof(std::pair<double,
                            typeof sensor_data>));
                        }
                    }


                    ImPlot::EndPlot();
                }
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

                    union {
                        struct {
                            int16_t acc_x, acc_y, acc_z;
                            int16_t mag_x, mag_y, mag_z;
                            int16_t gyro_x, gyro_y, gyro_z;
                            int16_t euler_x, euler_y, euler_z;
                            int16_t quat_w, quat_x, quat_y, quat_z;
                            int16_t lin_acc_x, lin_acc_y, lin_acc_z;
                            int16_t grav_x, grav_y, grav_z;
                            int8_t temp;
                        };
                        uint8_t data[45];
                    } unscaled{};
                    std::copy(buffer.begin() + 1, buffer.end(), unscaled.data);

                    sensor_data.acc = {unscaled.acc_x / 100.0, unscaled.acc_y / 100.0, unscaled.acc_z / 100.0};
                    sensor_data.mag = {unscaled.mag_x / 16.0, unscaled.mag_y / 16.0, unscaled.mag_z / 16.0};
                    sensor_data.gyro = {unscaled.gyro_x / 16.0, unscaled.gyro_y / 16.0, unscaled.gyro_z / 16.0};
                    sensor_data.euler = {unscaled.euler_x / 16.0, unscaled.euler_y / 16.0, unscaled.euler_z / 16.0};
                    sensor_data.quat = {unscaled.quat_w / 16384.0, unscaled.quat_x / 16384.0, unscaled.quat_y / 16384.0, unscaled.quat_z / 16384.0};
                    sensor_data.lin_acc = {unscaled.lin_acc_x / 100.0, unscaled.lin_acc_y / 100.0, unscaled.lin_acc_z / 100.0};
                    sensor_data.grav = {unscaled.grav_x / 100.0, unscaled.grav_y / 100.0, unscaled.grav_z / 100.0};
                    sensor_data.temp = unscaled.temp;

                    sensor_history.emplace_back(ImGui::GetTime(), sensor_data);

                    if (sensor_history.size() > 300) {
                        sensor_history.erase(sensor_history.begin());
                    }
                    break;
            }
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));



        ImGui::Begin("Camera Stream");

        ImVec2 windowSize = ImGui::GetWindowSize();
        windowSize.y -= ImGui::GetFrameHeight();

        ImVec2 imageSize((float) videoWidth, (float) videoHeight);

        if (windowSize.x / windowSize.y > imageSize.x / imageSize.y) {
            imageSize.x = windowSize.y * imageSize.x / imageSize.y;
            imageSize.y = windowSize.y;
            ImGui::SetCursorPosX((windowSize.x - imageSize.x) / 2);
        } else {
            imageSize.y = windowSize.x * imageSize.y / imageSize.x;
            imageSize.x = windowSize.x;
            ImGui::SetCursorPosY(ImGui::GetFrameHeight() + (windowSize.y - imageSize.y) / 2);
        }

        ImGui::Image((void*)(intptr_t)videoTexture, imageSize, ImVec2(0, 0), ImVec2(1, 1));

        ImGui::End();
        ImGui::PopStyleVar();

        ImGui::Begin("Thruster Control");

        bool controlDataChanged = false;
        ImGui::Checkbox("Use Controller", &shouldUseController);
        if (!shouldUseController) {
            {
                const ImVec2 key_size = ImVec2(35.0f, 35.0f);
                const float  key_rounding = 3.0f;
                const ImVec2 key_face_size = ImVec2(25.0f, 25.0f);
                const ImVec2 key_face_pos = ImVec2(5.0f, 3.0f);
                const float  key_face_rounding = 2.0f;
                const ImVec2 key_label_pos = ImVec2(7.0f, 4.0f);
                const ImVec2 key_step = ImVec2(key_size.x - 1.0f, key_size.y - 1.0f);
                const float  key_row_offset = 9.0f;

                ImVec2 board_min = ImGui::GetCursorScreenPos();
                ImVec2 board_max = ImVec2(board_min.x + 9 * key_step.x + 2 * key_row_offset + 10.0f, board_min.y + 3 * key_step.y + 10.0f);
                ImVec2 start_pos = ImVec2(board_min.x + 5.0f - key_step.x, board_min.y);

                struct KeyLayoutData { int Row, Col; const char* Label; ImGuiKey Key; };
                const KeyLayoutData keys_to_display[] = {
                        { 0, 0, "", ImGuiKey_Tab },      { 0, 1, "Q", ImGuiKey_Q }, { 0, 2, "W", ImGuiKey_W }, { 0, 3, "E", ImGuiKey_E }, { 0, 4, "R", ImGuiKey_R }, { 0, 5, "T", ImGuiKey_T }, { 0, 6, "Y", ImGuiKey_Y }, { 0, 7, "U", ImGuiKey_U }, { 0, 8, "I", ImGuiKey_I }, { 0, 9, "O", ImGuiKey_O }, { 0, 10, "P", ImGuiKey_P },
                        { 1, 0, "", ImGuiKey_CapsLock }, { 1, 1, "A", ImGuiKey_A }, { 1, 2, "S", ImGuiKey_S }, { 1, 3, "D", ImGuiKey_D }, { 1, 4, "F", ImGuiKey_F }, { 1, 5, "G", ImGuiKey_G }, { 1, 6, "H", ImGuiKey_H }, { 1, 7, "J", ImGuiKey_J }, { 1, 8, "K", ImGuiKey_K }, { 1, 9, "L", ImGuiKey_L }, { 1, 10, "", ImGuiKey_Enter },
                        { 2, 0, "", ImGuiKey_LeftShift },{ 2, 1, "Z", ImGuiKey_Z }, { 2, 2, "X", ImGuiKey_X }, { 2, 3, "C", ImGuiKey_C }, { 2, 4, "V", ImGuiKey_V }, { 2, 5, "B", ImGuiKey_B }, { 2, 6, "N", ImGuiKey_N }, { 2, 7, "M", ImGuiKey_M }, { 2, 8, ",", ImGuiKey_Comma }, { 2, 9, ".", ImGuiKey_Period }, { 2, 10, "/", ImGuiKey_Slash },
                };

                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                draw_list->PushClipRect(board_min, board_max, true);
                for (int n = 0; n < IM_ARRAYSIZE(keys_to_display); n++)
                {
                    const KeyLayoutData* key_data = &keys_to_display[n];
                    ImVec2 key_min = ImVec2(start_pos.x + key_data->Col * key_step.x + key_data->Row * key_row_offset, start_pos.y + key_data->Row * key_step.y);
                    ImVec2 key_max = ImVec2(key_min.x + key_size.x, key_min.y + key_size.y);
                    draw_list->AddRectFilled(key_min, key_max, IM_COL32(204, 204, 204, 255), key_rounding);
                    draw_list->AddRect(key_min, key_max, IM_COL32(24, 24, 24, 255), key_rounding);
                    ImVec2 face_min = ImVec2(key_min.x + key_face_pos.x, key_min.y + key_face_pos.y);
                    ImVec2 face_max = ImVec2(face_min.x + key_face_size.x, face_min.y + key_face_size.y);
                    draw_list->AddRect(face_min, face_max, IM_COL32(193, 193, 193, 255), key_face_rounding, ImDrawFlags_None, 2.0f);
                    draw_list->AddRectFilled(face_min, face_max, IM_COL32(252, 252, 252, 255), key_face_rounding);
                    ImVec2 label_min = ImVec2(key_min.x + key_label_pos.x, key_min.y + key_label_pos.y);
                    draw_list->AddText(label_min, IM_COL32(64, 64, 64, 255), key_data->Label);
                    if (ImGui::IsKeyDown(key_data->Key))
                        draw_list->AddRectFilled(key_min, key_max, IM_COL32(255, 0, 0, 128), key_rounding);
                }
                draw_list->PopClipRect();
                ImGui::Dummy(ImVec2(board_max.x - board_min.x, board_max.y - board_min.y));
            }

            auto bindThrusterKey = [&](ImGuiKey key, float value, float& thruster) {
                if (ImGui::IsKeyPressed(key, false)) {
                    thruster = value;
                    controlDataChanged = true;
                } else if (ImGui::IsKeyReleased(key)) {
                    thruster = 0;
                    controlDataChanged = true;
                }
            };

            bindThrusterKey(ImGuiKey_W, thrusterData.speed, thrusterData.front);
            bindThrusterKey(ImGuiKey_S, -thrusterData.speed, thrusterData.front);
            bindThrusterKey(ImGuiKey_A, -thrusterData.speed, thrusterData.side);
            bindThrusterKey(ImGuiKey_D, thrusterData.speed, thrusterData.side);
            bindThrusterKey(ImGuiKey_Space, thrusterData.speed, thrusterData.up);
            bindThrusterKey(ImGuiKey_LeftShift, -thrusterData.speed, thrusterData.up);
            bindThrusterKey(ImGuiKey_O, thrusterData.speed, thrusterData.pitch);
            bindThrusterKey(ImGuiKey_U, -thrusterData.speed, thrusterData.pitch);
            bindThrusterKey(ImGuiKey_J, -thrusterData.speed, thrusterData.roll);
            bindThrusterKey(ImGuiKey_L, thrusterData.speed, thrusterData.roll);
            bindThrusterKey(ImGuiKey_Q, -thrusterData.speed, thrusterData.yaw);
            bindThrusterKey(ImGuiKey_E, thrusterData.speed, thrusterData.yaw);
        } else {
            bool isConnected = glfwJoystickPresent(GLFW_JOYSTICK_1);
            if (isConnected) {
                ImGui::Text("Joystick Connected");

                int axesCount;
                const float* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &axesCount);

                float currXAxis = axes[0], currYAxis = axes[1] * -1;

                ImGui::Text("X: %f, Y: %f", currXAxis, currYAxis);

                if (xAxis != currXAxis) {
                    xAxis = currXAxis;
                    controlDataChanged = true;
                    thrusterData.front = (thrusterData.speed * xAxis);
                }

                if (yAxis != currYAxis) {
                    yAxis = currYAxis;
                    controlDataChanged = true;
                    thrusterData.side = (thrusterData.speed * yAxis);
                }
            } else {
                ImGui::Text("Joystick Not Connected");
            }
        }

        if (controlDataChanged) {
            std::vector<uint8_t> thruster_buf(25);
            thruster_buf[0] = 0x02;
            std::copy(thrusterData.data, thrusterData.data + 24, thruster_buf.begin() + 1);

            communication.send(thruster_buf);
        }

        static float clawSpeeds[2] = {0.01f, 0.01f};
        ImGui::SliderFloat2("Claw Speed", clawSpeeds, 0.01f, 0.2f);

        ImGui::Separator();

//        if (ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_Enter, false)) {
//            communication.send({0x04});
//        }

        if (ImGui::IsKeyPressed(ImGuiKey_Equal, false)) thrusterData.speed++;
        if (ImGui::IsKeyPressed(ImGuiKey_Minus, false)) thrusterData.speed--;
        if (ImGui::IsKeyPressed(ImGuiKey_0, false)) thrusterData.speed = 0;
        if (ImGui::IsKeyPressed(ImGuiKey_1, false)) thrusterData.speed = 1;
        if (ImGui::IsKeyPressed(ImGuiKey_2, false)) thrusterData.speed = 2;
        if (ImGui::IsKeyPressed(ImGuiKey_3, false)) thrusterData.speed = 3;
        if (ImGui::IsKeyPressed(ImGuiKey_4, false)) thrusterData.speed = 4;

        thrusterData.speed = std::clamp(thrusterData.speed, 0.0f, 40.0f);
        if (ImGui::SliderFloat("Speed", &thrusterData.speed, 0, 40, "%.2f")) controlDataChanged = true;

        bool gripChanged = false;
        if (ImGui::IsKeyPressed(ImGuiKey_K)) {
            claw0 = std::clamp(claw0 + clawSpeeds[0], -1.0f, 1.0f);
            gripChanged = true;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_I)) {
            claw0 = std::clamp(claw0 - clawSpeeds[0], -1.0f, 1.0f);
            gripChanged = true;
        }
        if (ImGui::SliderFloat("Grip", &claw0, -1, 1, "%.2f")) gripChanged = true;
        if (gripChanged) {
            auto buf = (uint8_t*) &claw0;
            communication.send({0x04, buf[0], buf[1], buf[2], buf[3]});
        }

        bool rotChanged = false;
        if (ImGui::IsKeyPressed(ImGuiKey_U)) {
            claw1 = std::clamp(claw1 + clawSpeeds[1], -1.0f, 1.0f);
            rotChanged = true;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_O)) {
            claw1 = std::clamp(claw1 - clawSpeeds[1], -1.0f, 1.0f);
            rotChanged = true;
        }
        if (ImGui::SliderFloat("Rotation", &claw1, -1, 1, "%.2f")) rotChanged = true;
        if (rotChanged) {
            auto buf = (uint8_t*) &claw1;
            communication.send({0x05, buf[0], buf[1], buf[2], buf[3]});
        }



        ImGui::End();

        ImGui::Begin("Reconstruction");

        if (ImGui::SliderInt("Recording Frame Rate", &imageFrameRate, 1, 30)) {
            GstCaps* caps = gst_caps_new_simple("video/x-raw", "framerate", GST_TYPE_FRACTION, imageFrameRate, 1, nullptr);
            g_object_set(imageRateFilter, "caps", caps, nullptr);
            gst_caps_unref(caps);
        }

        static int cropBottom = 0;
        if (ImGui::SliderInt("Bottom Crop Pixels", &cropBottom, 0, 500)) {
            g_object_set(imageCrop, "bottom", cropBottom, nullptr);
        }

        if (!imageRecording && ImGui::Button("Start Recording")) {
            std::filesystem::remove_all("images");
            std::filesystem::create_directory("images");

            gst_element_send_event (pipeline, gst_event_new_flush_start ());
            gst_element_send_event (pipeline, gst_event_new_flush_stop(true));
            g_object_set(imageSink, "index", 0, nullptr);
//            gst_element_set_state(imageQueue, GST_STATE_PLAYING);
//            gst_element_link(tee, imageQueue);
//            gst_element_set_state(imageQueue, GST_STATE_PLAYING);
            g_object_set(imageValve, "drop", false, nullptr);

            imageRecording = true;
        } else if (imageRecording && ImGui::Button("Stop Recording")) {
            g_object_set(imageValve, "drop", true, nullptr);
//            gst_element_unlink(tee, imageQueue);
//            gst_element_set_state(imageQueue, GST_STATE_NULL);
            imageRecording = false;
        }

        ImGui::Separator();
        ImGui::Spacing();

        ImGui::ProgressBar((float) get_progress(), ImVec2(0.0f, 0.0f));
        ImGui::SameLine();
        ImGui::Text("ETA: %0.2fs", get_eta());

        if (ImGui::Button("Start Photogrammetry Session")) {
            if (std::filesystem::is_empty("images")) {
                std::cerr << "Error: No images to process" << std::endl;
            } else {
                run_photogrammetry_session("images");
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Open Model")) {
            std::system("open out.usdz");
        }
        ImGui::End();

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + 10.0f, viewport->WorkSize.y + viewport->WorkPos.y - 10.0f), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::SetNextWindowBgAlpha(0.35f);
        ImGui::Begin("Overlay", nullptr, window_flags);
        static bool stopwatchRunning = false;
        static std::chrono::system_clock::time_point stopwatchStart;
        static std::chrono::duration<float, std::milli> stopwatchDuration;

        auto minutes = std::chrono::duration_cast<std::chrono::minutes>(stopwatchDuration);
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(stopwatchDuration - minutes);
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(stopwatchDuration - minutes - seconds);

        ImGui::Text("%02ld:%02lld:%03lld", minutes.count(), seconds.count(), milliseconds.count());

        if (stopwatchRunning) {
            stopwatchDuration = std::chrono::system_clock::now() - stopwatchStart;

            if (ImGui::Button("Pause")) stopwatchRunning = false;
        } else {
            if (ImGui::Button("Start")) {
                stopwatchRunning = true;
                if (stopwatchDuration.count() == 0)
                    stopwatchStart = std::chrono::system_clock::now();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
            stopwatchRunning = false;
            stopwatchDuration = std::chrono::duration<float, std::milli>(0);
        }

        ImGui::End();

        ImGui::Begin("Sturgeon Analyzer");
        static struct {
            std::vector<int> times;
            std::vector<std::pair<std::string, std::vector<int>>> values;
        } surgeonData;
        static NFD::UniquePath outPath;
        if (ImGui::Button("Open File")) {
            nfdfilteritem_t filters[1] = {{ "Data", "csv" }};
            if (NFD::OpenDialog(outPath, filters, 1) == NFD_OKAY) {
                std::ifstream file(outPath.get());

                surgeonData = {};

                std::string line;
                std::getline(file, line);

                std::stringstream ss(line);
                std::string token;
                std::getline(ss, token, ',');

                std::vector<int> values;
                while (std::getline(ss, token, ',')) {
                    surgeonData.times.push_back(std::stoi(token));
                }

                for (int i = 0; std::getline(file, line); i++) {
                    ss = std::stringstream(line);
                    token = "";
                    std::getline(ss, token, ',');
                    surgeonData.values.emplace_back(token, std::vector<int>());
                    for (int j = 0; j < surgeonData.times.size(); j++) {
                        std::getline(ss, token, ',');
                        surgeonData.values[i].second.push_back(std::stoi(token));
                    }
                }

                file.close();
            }

        }

        if (!surgeonData.values.empty() && ImPlot::BeginPlot("Sturgeon Analyzer", ImVec2(-1, 0))) {
            for (auto &[name, values] : surgeonData.values) {
                ImPlot::PlotLine(name.c_str(), surgeonData.times.data(), values.data(), surgeonData.times.size(), 0, 0, sizeof(int));
            }
            ImPlot::EndPlot();
        }

        ImGui::End();

        ImGui::Render();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
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

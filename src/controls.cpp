#include <imgui.h>

#include "controls.h"
#include "depth_sensor.h"

Controls::Controls(Pi &pi) : pi_(pi) {
    pi.SetServoPulseWidth(Thruster::FRONT_LEFT, 1500);
    pi.SetServoPulseWidth(Thruster::FRONT_RIGHT, 1500);
    pi.SetServoPulseWidth(Thruster::REAR_LEFT, 1500);
    pi.SetServoPulseWidth(Thruster::REAR_RIGHT, 1500);
    pi.SetServoPulseWidth(Thruster::MID_LEFT, 1500);
    pi.SetServoPulseWidth(Thruster::MID_RIGHT, 1500);
}

Controls::~Controls() {
    if (controls_thread_running_) {
        controls_thread_running_ = false;
        controls_thread_.join();
    }

    pi_.SetServoPulseWidth(Thruster::FRONT_LEFT, 1500);
    pi_.SetServoPulseWidth(Thruster::FRONT_RIGHT, 1500);
    pi_.SetServoPulseWidth(Thruster::REAR_LEFT, 1500);
    pi_.SetServoPulseWidth(Thruster::REAR_RIGHT, 1500);
    pi_.SetServoPulseWidth(Thruster::MID_LEFT, 1500);
    pi_.SetServoPulseWidth(Thruster::MID_RIGHT, 1500);
}

void Controls::ShowControlsWindow() {
    static bool use_controller = false;

    ImGui::Begin("Controls");
    ImGui::Checkbox("Use Controller", &use_controller);

    if (!use_controller) {
        Controls::DrawKeyboard();

        Controls::BindThrusterKey(this->movement_vector_.forward, ImGuiKey_W, ImGuiKey_S);
        Controls::BindThrusterKey(this->movement_vector_.side, ImGuiKey_D, ImGuiKey_A);
        Controls::BindThrusterKey(this->movement_vector_.up, ImGuiKey_Space, ImGuiKey_LeftShift);
        Controls::BindThrusterKey(this->movement_vector_.pitch, ImGuiKey_O, ImGuiKey_U);
        Controls::BindThrusterKey(this->movement_vector_.roll, ImGuiKey_L, ImGuiKey_J);
        Controls::BindThrusterKey(this->movement_vector_.yaw, ImGuiKey_E, ImGuiKey_Q);

        ImGui::SliderFloat("Speed", reinterpret_cast<float *>(&speed_), 0.0, 30.0);

        ImGui::SliderFloat("Target Depth", reinterpret_cast<float *>(&target_depth_), 0.0, 30.0);
    } else {
        ImGui::Text("todo!");
    }

    ImGui::End();
}

void Controls::UpdateThrusters(const DepthSensor &depth_sensor) {
    double front_right = speed_ * (this->movement_vector_.forward - this->movement_vector_.side - this->movement_vector_.yaw) / 30.0;
    double front_left = speed_ * (this->movement_vector_.forward + this->movement_vector_.side + this->movement_vector_.yaw) / 30.0;
    double back_left = speed_ * -(this->movement_vector_.forward - this->movement_vector_.side + this->movement_vector_.yaw) / 30.0;
    double back_right = speed_ * -(this->movement_vector_.forward + this->movement_vector_.side - this->movement_vector_.yaw) / 30.0;
    double mid_left = speed_ * -(this->movement_vector_.up - this->movement_vector_.roll) / 20.0;
    double mid_right = speed_ * -(this->movement_vector_.up + this->movement_vector_.roll) / 20.0;

    if (this->movement_vector_.up == 0) {
        if (target_depth_ == -1.0) {
            target_depth_ = depth_sensor.GetDepth();
            prev_time_ = std::chrono::steady_clock::now();
        }

        double depth_error = target_depth_ - depth_sensor.GetDepth();

        auto current_time = std::chrono::steady_clock::now();
        auto dt = std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - prev_time_).count();
        prev_time_ = current_time;

        double p = k_proportional_ * depth_error;
        double i = k_integral_ * ;
        double d = k_derivative_ * depth_error / dt;

        int output = (int) std::clamp(p + i + d, 1000.0, 2000.0);

        pi_.SetServoPulseWidth(Thruster::MID_LEFT, output);
        pi_.SetServoPulseWidth(Thruster::MID_RIGHT, output);

    } else {
        target_depth_ = -1.0;

        pi_.SetServoPulseWidth(Thruster::MID_LEFT, Controls::DoubleToPulseWidth(mid_left));
        pi_.SetServoPulseWidth(Thruster::MID_RIGHT, Controls::DoubleToPulseWidth(mid_right));
    }


    pi_.SetServoPulseWidth(Thruster::FRONT_LEFT, Controls::DoubleToPulseWidth(front_left));
    pi_.SetServoPulseWidth(Thruster::FRONT_RIGHT, Controls::DoubleToPulseWidth(front_right));
    pi_.SetServoPulseWidth(Thruster::REAR_LEFT, Controls::DoubleToPulseWidth(back_left));
    pi_.SetServoPulseWidth(Thruster::REAR_RIGHT, Controls::DoubleToPulseWidth(back_right));

}

uint16_t Controls::DoubleToPulseWidth(double value) {
    return std::clamp((uint16_t) (value + 1) * 500 + 1000, 1000, 2000);
}

bool Controls::BindThrusterKey(double &axis, ImGuiKey positive, ImGuiKey negative) {
    bool changed = false;

    if (ImGui::IsKeyPressed(positive, false)) {
        axis += 1.0;
        changed = true;
    } else if (ImGui::IsKeyReleased(positive)) {
        axis -= 1.0;
        changed = true;
    }

    if (ImGui::IsKeyPressed(negative, false)) {
        axis -= 1.0;
        changed = true;
    } else if (ImGui::IsKeyReleased(negative)) {
        axis += 1.0;
        changed = true;
    }

    axis = std::clamp(axis, -1.0, 1.0);
    return changed;
}

void Controls::DrawKeyboard() {
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
    for (const auto & n : keys_to_display) {
        const KeyLayoutData* key_data = &n;
        ImVec2 key_min = ImVec2(start_pos.x + (float) key_data->Col * key_step.x + (float) key_data->Row * key_row_offset, start_pos.y + key_data->Row * key_step.y);
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

void Controls::StartControlsThread(const DepthSensor &depth_sensor) {
    controls_thread_running_ = true;
    controls_thread_ = std::thread(&Controls::ControlsThreadLoop, this, std::ref(depth_sensor));
}

void Controls::ControlsThreadLoop(const DepthSensor &depth_sensor) {
    while (controls_thread_running_) {
        UpdateThrusters(depth_sensor);
    }
}

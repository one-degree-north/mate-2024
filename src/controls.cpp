#include <imgui.h>

#include "controls.h"
#include "depth_sensor.h"
#include "orientation_sensor.h"
#include <iostream>
Controls::Controls(Pi &pi) : pi_(pi) {
    feather_wing_handle_ = pi_.OpenI2C(1, 0x40);

    pi_.WriteI2CByteData(feather_wing_handle_, 0x00, 0x10);
    pi_.WriteI2CByteData(feather_wing_handle_, 0xFE, 0x7D); // set featherwing pwm frequency to as close to 50hz as possible
    pi_.WriteI2CByteData(feather_wing_handle_, 0x00, 0x00);

    std::this_thread::sleep_for(std::chrono::milliseconds (100));

    union {
        uint32_t thrusts[8];
        char feather_wing_data[32];
    } data {};

    for (uint32_t &thrust : data.thrusts) {
        thrust = Controls::DoubleToFeatherWingOnTime(0) << 16;
    }

//    int res = pi_.WriteI2CBlockData(feather_wing_handle_, 0x06, data.feather_wing_data, 32);
    for (int i = 0; i < 32; i++)
        pi_.WriteI2CByteData(feather_wing_handle_, 0x06 + i, data.feather_wing_data[i]);
//    std::cout << res << std::endl;
}

Controls::~Controls() {
    if (controls_thread_running_) {
        controls_thread_running_ = false;
        controls_thread_.join();
    }

    union {
        uint32_t thrusts[8];
        char feather_wing_data[32];
    } data {};

    for (uint32_t &thrust : data.thrusts) {
        thrust = Controls::DoubleToFeatherWingOnTime(0) << 16;
    }

//    int res = pi_.WriteI2CBlockData(feather_wing_handle_, 0x06, data.feather_wing_data, 32);
    for (int i = 0; i < 32; i++)
        pi_.WriteI2CByteData(feather_wing_handle_, 0x06 + i, data.feather_wing_data[i]);

    pi_.CloseI2C(feather_wing_handle_);
}

void Controls::ShowControlsWindow() {
    ImGui::Begin("Controls");

    if (ImGui::Checkbox("PID Enabled", &pid_enabled_) && pid_enabled_) {
        this->depth_pid_.Reset();
        this->roll_pid_.Reset();
        this->pitch_pid_.Reset();
        this->yaw_pid_.Reset();
    }

    ImGui::Checkbox("Use Controller", &use_controller_);

    if (!use_controller_) {
        Controls::DrawKeyboard();

        Controls::BindThrusterKey(this->movement_vector_.forward, ImGuiKey_W, ImGuiKey_S);
        Controls::BindThrusterKey(this->movement_vector_.side, ImGuiKey_D, ImGuiKey_A);

        if (!pid_enabled_) {
            Controls::BindThrusterKey(this->movement_vector_.up, ImGuiKey_Space, ImGuiKey_LeftShift);
            Controls::BindThrusterKey(this->movement_vector_.pitch, ImGuiKey_O, ImGuiKey_U);
            Controls::BindThrusterKey(this->movement_vector_.roll, ImGuiKey_L, ImGuiKey_J);
            Controls::BindThrusterKey(this->movement_vector_.yaw, ImGuiKey_E, ImGuiKey_Q);
        } else {
            if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) this->depth_pid_.SetTarget(this->depth_pid_.GetTarget() + 0.01);
            if (ImGui::IsKeyPressed(ImGuiKey_LeftShift, false)) this->depth_pid_.SetTarget(this->depth_pid_.GetTarget() - 0.01);
            if (ImGui::IsKeyPressed(ImGuiKey_O, false)) this->pitch_pid_.SetTarget(this->pitch_pid_.GetTarget() + 0.01);
            if (ImGui::IsKeyPressed(ImGuiKey_U, false)) this->pitch_pid_.SetTarget(this->pitch_pid_.GetTarget() - 0.01);
            if (ImGui::IsKeyPressed(ImGuiKey_L, false)) this->roll_pid_.SetTarget(this->roll_pid_.GetTarget() + 0.01);
            if (ImGui::IsKeyPressed(ImGuiKey_J, false)) this->roll_pid_.SetTarget(this->roll_pid_.GetTarget() - 0.01);
            if (ImGui::IsKeyPressed(ImGuiKey_E, false)) this->yaw_pid_.SetTarget(this->yaw_pid_.GetTarget() + 0.01);
            if (ImGui::IsKeyPressed(ImGuiKey_Q, false)) this->yaw_pid_.SetTarget(this->yaw_pid_.GetTarget() - 0.01);
        }

        ImGui::Text("Movement Vector");
        ImGui::Text("Forward: %f", movement_vector_.up);


        ImGui::SliderFloat("Speed", reinterpret_cast<float *>(&this->speed_), 0.0, 30.0);

        ImGui::SliderFloat("Target Depth", reinterpret_cast<float *>(&this->target_depth_), 0.0, 30.0);

        if (ImGui::Button("Reset")) {
            this->depth_pid_.Reset();
            this->roll_pid_.Reset();
            this->pitch_pid_.Reset();
            this->yaw_pid_.Reset();
        }

//        ImGui::BeginCombo("PID Config", nullptr);
//
//        if (ImGui::Selectable("Depth")) { this->depth_pid_.DrawPIDConfigWindow(); }
//        if (ImGui::Selectable("Roll")) { this->roll_pid_.DrawPIDConfigWindow(); }
//        if (ImGui::Selectable("Pitch")) { this->pitch_pid_.DrawPIDConfigWindow(); }
//        if (ImGui::Selectable("Yaw")) { this->yaw_pid_.DrawPIDConfigWindow(); }
//
//        ImGui::EndCombo();


    } else {
        ImGui::Text("todo!");
    }

    ImGui::End();
}

void Controls::UpdateThrusters(const DepthSensor &depth_sensor, const OrientationSensor &orientation_sensor) {
    if (pid_enabled_) {
        movement_vector_.up = depth_pid_.Update(depth_sensor.GetDepth());
        movement_vector_.yaw = yaw_pid_.Update(orientation_sensor.GetOrientationYaw());
        movement_vector_.roll = roll_pid_.Update(orientation_sensor.GetOrientationRoll());
        movement_vector_.pitch = pitch_pid_.Update(orientation_sensor.GetOrientationPitch());
    }

    double front_left = speed_ * (movement_vector_.forward + movement_vector_.side + movement_vector_.yaw) / 30.0;
    double front_right = speed_ * (movement_vector_.forward - movement_vector_.side - movement_vector_.yaw) / 30.0;
    double back_left = -speed_ * (movement_vector_.forward - movement_vector_.side + movement_vector_.yaw) / 30.0;
    double back_right = -speed_ * (movement_vector_.forward + movement_vector_.side - movement_vector_.yaw) / 30.0;
    double mid_front_left = -speed_ * (movement_vector_.up - movement_vector_.roll + movement_vector_.pitch) / 30.0;
    double mid_front_right = -speed_ * (movement_vector_.up + movement_vector_.roll + movement_vector_.pitch) / 30.0;
    double mid_back_left = -speed_ * (movement_vector_.up - movement_vector_.roll - movement_vector_.pitch) / 30.0;
    double mid_back_right = -speed_ * (movement_vector_.up + movement_vector_.roll - movement_vector_.pitch) / 30.0;

    union {
        uint32_t thrusts[8];
        char feather_wing_register_data[32];
    } data {};


    data.thrusts[Thruster::FRONT_LEFT] = Controls::DoubleToFeatherWingOnTime(front_left);
    data.thrusts[Thruster::FRONT_RIGHT] = Controls::DoubleToFeatherWingOnTime(front_right);
    data.thrusts[Thruster::REAR_LEFT] = Controls::DoubleToFeatherWingOnTime(back_left);
    data.thrusts[Thruster::REAR_RIGHT] = Controls::DoubleToFeatherWingOnTime(back_right);
    data.thrusts[Thruster::MID_FRONT_LEFT] = Controls::DoubleToFeatherWingOnTime(mid_front_left);
    data.thrusts[Thruster::MID_FRONT_RIGHT] = Controls::DoubleToFeatherWingOnTime(mid_front_right);
    data.thrusts[Thruster::MID_BACK_LEFT] = Controls::DoubleToFeatherWingOnTime(mid_back_left);
    data.thrusts[Thruster::MID_BACK_RIGHT] = Controls::DoubleToFeatherWingOnTime(mid_back_right);

    for (uint32_t &thrust : data.thrusts)
        thrust <<= 16;


//    int res = pi_.WriteI2CBlockData(feather_wing_handle_, 0x06, data.feather_wing_data, 32);
    for (int i = 0; i < 32; i++)
        pi_.WriteI2CByteData(feather_wing_handle_, 0x06 + i, data.feather_wing_register_data[i]);

//    pi_.SetServoPulseWidth(Thruster::MID_FRONT_LEFT, Controls::DoubleToPulseWidth(mid_front_left));
//    pi_.SetServoPulseWidth(Thruster::MID_FRONT_RIGHT, Controls::DoubleToPulseWidth(mid_front_right));
//    pi_.SetServoPulseWidth(Thruster::MID_BACK_LEFT, Controls::DoubleToPulseWidth(mid_back_left));
//    pi_.SetServoPulseWidth(Thruster::MID_BACK_RIGHT, Controls::DoubleToPulseWidth(mid_back_right));
//    pi_.SetServoPulseWidth(Thruster::FRONT_LEFT, Controls::DoubleToPulseWidth(front_left));
//    pi_.SetServoPulseWidth(Thruster::FRONT_RIGHT, Controls::DoubleToPulseWidth(front_right));
//    pi_.SetServoPulseWidth(Thruster::REAR_LEFT, Controls::DoubleToPulseWidth(back_left));
//    pi_.SetServoPulseWidth(Thruster::REAR_RIGHT, Controls::DoubleToPulseWidth(back_right));
}

uint32_t Controls::DoubleToFeatherWingOnTime(double value) {
//    return std::clamp((uint16_t) (value + 1) * 500 + 1000, 1000, 2000);
    return std::clamp((uint16_t) (value * 102) + 307, 205, 409); // from (-1, 1) to (205, 409) (out of 4096, 1ms to 2ms @ 50hz)
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

void Controls::StartControlsThread(const DepthSensor &depth_sensor, const OrientationSensor &orientation_sensor) {
    controls_thread_running_ = true;
    controls_thread_ = std::thread(&Controls::ControlsThreadLoop, this, std::ref(depth_sensor), std::ref(orientation_sensor));
}

void Controls::ControlsThreadLoop(const DepthSensor &depth_sensor, const OrientationSensor &orientation_sensor) {
    while (controls_thread_running_) {
        UpdateThrusters(depth_sensor, orientation_sensor);

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

#include "imgui.h"

#include "orientation_sensor.h"

OrientationSensor::OrientationSensor(Pi &pi) : pi_(pi) {
    orientation_sensor_handle_ = pi_.OpenI2C(1, 0x28);
    if (orientation_sensor_handle_ < 0) {
        throw std::runtime_error("Failed to open orientation sensor I2C handle");
    }

    pi.WriteI2CByteData(orientation_sensor_handle_, 0x3F, 0x20);
    std::this_thread::sleep_for(std::chrono::milliseconds(650));

    pi.WriteI2CByteData(orientation_sensor_handle_, 0x3E, 0); // Normal Power Mode
    pi.WriteI2CByteData(orientation_sensor_handle_, 0x3F, 0); // Use Internal Oscillator

    pi.WriteI2CByteData(orientation_sensor_handle_, 0x3D, 0x0C); // Go to NDOF mode
    pi.WriteI2CByteData(orientation_sensor_handle_, 0x3B, 0); // Unit Selection
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
}

OrientationSensor::~OrientationSensor() {
    orientation_sensor_thread_running_ = false;
    orientation_sensor_thread_.join();

    pi_.CloseI2C(orientation_sensor_handle_);
}

void OrientationSensor::ShowOrientationSensorWindow() {
    ImGui::Begin("Orientation Sensor");

    ImGui::Text("Yaw: %f", orientation_yaw_.load());
    ImGui::Text("Roll: %f", orientation_roll_.load());
    ImGui::Text("Pitch: %f", orientation_pitch_.load());
    ImGui::Text("sys calib: %u", sys_calib_.load());
    ImGui::Text("gyr calib: %u", gyr_calib_.load());
    ImGui::Text("acc calib: %u", acc_calib_.load());
    ImGui::Text("mag calib: %u", mag_calib_.load());
    for (int i = 0; i < 22; i++){
        ImGui::Text("calib: %u, %u", i, actual_calib_data[i]);
    }
    if (ImGui::Button("plot yaw"))plot_yaw=!plot_yaw;
    if (ImGui::Button("plot pitch"))plot_pitch=!plot_pitch;
    if (ImGui::Button("plot roll"))plot_roll=!plot_roll;

    if (plot_yaw) ImGui::PlotLines("yaw: ", yaw_graph, 10000);
    if (plot_pitch) ImGui::PlotLines("pitch: ", pitch_graph, 10000);
    if (plot_roll) ImGui::PlotLines("roll: ", roll_graph, 10000);
    // ImGui::PlotLines("orientation: ", orientation_yaw_graph_, 10000);
    ImGui::End();
}

void OrientationSensor::StartOrientationSensorThread() {
    orientation_sensor_thread_running_ = true;
    orientation_sensor_thread_ = std::thread(&OrientationSensor::OrientationSensorThreadLoop, this);
}

void OrientationSensor::OrientationSensorThreadLoop() {
    while (orientation_sensor_thread_running_) {
        union {
            struct { int16_t yaw, roll, pitch; };
            char data[6];
        } buff {};
        pi_.ReadI2CBlockData(orientation_sensor_handle_, 0x1A, buff.data, 6);

        orientation_yaw_ = buff.yaw / 16.0;
        orientation_roll_ = buff.pitch / 16.0; // fliped roll and pitch since bno is rotated
        orientation_pitch_ = buff.roll / 16.0;

        

        calib_data_ = pi_.ReadI2CByteData(orientation_sensor_handle_, 0x35);
        sys_calib_ = (calib_data_ && 0b11000000) >> 6;
        gyr_calib_ = (calib_data_ && 0b00110000) >> 4;
        acc_calib_ = (calib_data_ && 0b00001100) >> 2;
        mag_calib_ = (calib_data_ && 0b00000011);

        pi_.ReadI2CBlockData(orientation_sensor_handle_, 0x55, actual_calib_data, 22);
        
        if (plot_yaw){
            for (int i = 0; i < 9999; i++){
                yaw_graph[i+1] = yaw_graph[i];
            }
            yaw_graph[0] = orientation_yaw_;
        }
        if (plot_roll){
            for (int i = 0; i < 9999; i++){
                roll_graph[i+1] = roll_graph[i];
            }
            roll_graph[0] = orientation_roll_;
        }
        if (plot_pitch){
            for (int i = 0; i < 9999; i++){
                pitch_graph[i+1] = pitch_graph[i];
            }
            pitch_graph[0] = orientation_pitch_;
        }
        // for (int i = 0; i < 9999; i++){
        //     orientation_yaw_graph_[i] = orientation_yaw_graph_[i+1];
        //     orientation_roll_graph_[i] = orientation_roll_graph_[i+1];
        //     orientation_pitch_graph_[i] = orientation_pitch_graph_[i+1];
        // }
        // orientation_yaw_graph_[9999] = orientation_yaw_;
        // orientation_roll_graph_[9999] = orientation_roll_;
        // orientation_pitch_graph_[9999] = orientation_pitch_;


        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

double OrientationSensor::GetOrientationYaw() const { return orientation_yaw_; }
double OrientationSensor::GetOrientationRoll() const { return orientation_roll_; }
double OrientationSensor::GetOrientationPitch() const { return orientation_pitch_; }

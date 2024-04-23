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

void OrientationSensor::ShowOrientationSensorWindow() const {
    ImGui::Begin("Orientation Sensor");

    ImGui::Text("Yaw: %f", orientation_yaw_.load());
    ImGui::Text("Roll: %f", orientation_roll_.load());
    ImGui::Text("Pitch: %f", orientation_pitch_.load());

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
        orientation_roll_ = buff.roll / 16.0;
        orientation_pitch_ = buff.pitch / 16.0;


        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

double OrientationSensor::GetOrientationYaw() const { return orientation_yaw_; }
double OrientationSensor::GetOrientationRoll() const { return orientation_roll_; }
double OrientationSensor::GetOrientationPitch() const { return orientation_pitch_; }

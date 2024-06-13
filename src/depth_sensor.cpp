#include <iostream>
#include <thread>

#include <imgui.h>
#include <implot.h>
#include "depth_sensor.h"
#include "pigpio.h"

DepthSensor::DepthSensor(Pi &pi) : pi_(pi) {
    pi.SetGPIOMode(2, PI_ALT0);
    pi.SetGPIOMode(3, PI_ALT0);
    this->pressure_sensor_handle_ = pi.OpenI2C(1, 0x76);

    if (pressure_sensor_handle_ < 0) {
        std::cerr << "Failed to open pressure sensor I2C handle" << std::endl;
        exit(1);
    }

    pi.WriteI2CByte(this->pressure_sensor_handle_, 0x1E); // Reset
    std::this_thread::sleep_for(std::chrono::milliseconds(11));

    for (int i = 0; i < 7; i++) {
        this->calibration_data_[i] = pi.ReadI2CWordData(this->pressure_sensor_handle_, 0xA0 + 2 * i);
        this->calibration_data_[i] = ((this->calibration_data_[i] & 0xFF) << 8) | (this->calibration_data_[i] >> 8);
    }


    for (int i = 0; i < 7; i++)
        printf("calibration_data_[%d]: %d\n", i, this->calibration_data_[i]);
}

DepthSensor::~DepthSensor() {
    if (depth_sensor_thread_running_) {
        depth_sensor_thread_running_ = false;
        depth_sensor_thread_.join();
    }

    pi_.CloseI2C(this->pressure_sensor_handle_);
}

void DepthSensor::PollSensorData() {
    uint32_t digital_pressure_value = 0, digital_temperature_value = 0;
    uint8_t value[3];

    pi_.WriteI2CByte(this->pressure_sensor_handle_, 0x44); // D1 Pressure conversion
    std::this_thread::sleep_for(std::chrono::microseconds(2500));
    pi_.ReadI2CBlockData(this->pressure_sensor_handle_, 0x00, (char*) value, 3);
    digital_pressure_value = (value[0] << 16) | (value[1] << 8) | value[2];

    pi_.WriteI2CByte(this->pressure_sensor_handle_, 0x54); // D2 Temperature conversion
    std::this_thread::sleep_for(std::chrono::microseconds(2500));
    pi_.ReadI2CBlockData(this->pressure_sensor_handle_, 0x00, (char*) value, 3);
    digital_temperature_value = (value[0] << 16) | (value[1] << 8) | value[2];

    int32_t dT = digital_temperature_value - uint32_t(this->calibration_data_[5]) * 256l;
    int32_t TEMP = 2000l + int64_t(dT) * this->calibration_data_[6] / 8388608LL;

    int64_t OFF = int64_t(this->calibration_data_[2]) * 65536l + (int64_t(this->calibration_data_[4]) * dT) / 128l;
    int64_t SENS = int64_t(this->calibration_data_[1]) * 32768l + (int64_t(this->calibration_data_[3]) * dT) / 256l;
    int32_t P = (digital_pressure_value* SENS / 2097152l - OFF) / (8192l);

    int64_t Ti, OFFi, SENSi;

    if ((TEMP / 100) < 20) {
        Ti = 3 * int64_t(dT) * int64_t(dT) / 8589934592LL;
        OFFi = (3 * (TEMP - 2000) * (TEMP - 2000)) / 2;
        SENSi = (5 * (TEMP - 2000) * (TEMP - 2000)) / 8;

        if ((TEMP / 100) < -15) {
            OFFi += 7 * (TEMP + 1500l) * (TEMP + 1500l);
            SENSi += 4 * (TEMP + 1500l) * (TEMP + 1500l);
        }
    } else {
        Ti = 2 * (dT * dT) / 137438953472LL;
        OFFi = ((TEMP - 2000) * (TEMP - 2000)) / 16;
        SENSi = 0;
    }

    this->temperature_ = (TEMP - Ti) / 100.0;
    this->pressure_ = (digital_pressure_value * (SENS - SENSi) / 2097152l - OFF + OFFi) / (8192l * 10.0);
}

void DepthSensor::ShowDepthSensorWindow() {
    ImGui::Begin("Depth Sensor");

    ImGui::Text("Temperature: %.2f C", this->temperature_.load());
    ImGui::Text("Pressure: %.2f mbar", this->pressure_.load());
    ImGui::Text("Depth: %.2f m", this->GetDepth());
    if (ImGui::Button("plot depth")) plot_depth=!plot_depth;
    if(ImPlot::BeginPlot("depth over time")){
        ImPlot::PlotLine("depth: ", depth_graph, 1500);
        ImPlot::EndPlot();
    }

    ImGui::End();
}

double DepthSensor::GetDepth() const {
    return -1*(this->pressure_.load() * 100.0 - 101300.0) / (1029 * 9.80665);
}

void DepthSensor::StartDepthSensorThread() {
    this->depth_sensor_thread_running_ = true;
    this->depth_sensor_thread_ = std::thread(&DepthSensor::DepthSensorThreadLoop, this);
}

void DepthSensor::DepthSensorThreadLoop() {
    while (this->depth_sensor_thread_running_) {
        PollSensorData();
        if (plot_depth){
            for (int i = 1499; i > 0; i--){
                depth_graph[i] = depth_graph[i-1];
            }
            depth_graph[0] = GetDepth();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

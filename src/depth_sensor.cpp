//
// Created by Sidharth Maheshwari on 18/4/24.
//
#include <thread>

#include <imgui.h>

#include "depth_sensor.h"

DepthSensor::DepthSensor(Pi &pi) : pi(pi) {
    pi.SetGPIOMode(2, 1);
    pi.SetGPIOMode(3, 1);
    this->pressure_sensor_handle = pi.OpenI2C(1, 0x76);

    pi.WriteI2CByte(this->pressure_sensor_handle, 0x1E); // Reset
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    pi.ReadI2CDevice(this->pressure_sensor_handle, (char*) this->CalibrationData, 14);

    for (uint16_t & i : this->CalibrationData)
        i = ((i & 0xFF) << 8) | (i >> 8);

    this->data_thread = std::thread(&DepthSensor::SensorDataThread, this);
}

DepthSensor::~DepthSensor() {
    pi.CloseI2C(this->pressure_sensor_handle);
    this->reading_sensor_data = false;
    this->data_thread.join();
}

void DepthSensor::SensorDataThread() {
    while (this->reading_sensor_data) {
        this->ReadSensorData();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

void DepthSensor::ReadSensorData() {
    uint32_t digital_pressure_value = 0, digital_temperature_value = 0;

    pi.WriteI2CByte(this->pressure_sensor_handle, 0x48); // D1 Pressure conversion
    std::this_thread::sleep_for(std::chrono::milliseconds(12));
    pi.ReadI2CDevice(this->pressure_sensor_handle, (char*) &digital_pressure_value, 3);
    digital_pressure_value = ((digital_pressure_value & 0xFF0000) >> 16) | (digital_pressure_value & 0x00FF00) | ((digital_pressure_value & 0x0000FF) << 16);

    pi.WriteI2CByte(this->pressure_sensor_handle, 0x58); // D2 Temperature conversion
    std::this_thread::sleep_for(std::chrono::milliseconds(12));
    pi.ReadI2CDevice(this->pressure_sensor_handle, (char*) &digital_temperature_value, 3);
    digital_temperature_value = ((digital_temperature_value & 0xFF0000) >> 16) | (digital_temperature_value & 0x00FF00) | ((digital_temperature_value & 0x0000FF) << 16);

    int32_t dT = digital_temperature_value - this->CalibrationData[5] * (1 << 8);
    int32_t TEMP = 2000 + dT * this->CalibrationData[6] / (1 << 23);

    int64_t OFF = this->CalibrationData[2] * (1 << 16) + (this->CalibrationData[4] * dT) / (1 << 7);
    int64_t SENS = this->CalibrationData[1] * (1 << 15) + (this->CalibrationData[3] * dT) / (1 << 8);
    int32_t P = (digital_pressure_value * (SENS / (1 << 21)) - OFF) / (1 << 15);

    this->P = P;
    this->TEMP = TEMP;

    int64_t Ti, OFFi, SENSi;

    if (TEMP < 2000) {
        Ti = 3 * pow(dT, 2) / pow(2, 33);
        OFFi = 3 * pow((TEMP - 2000), 2) / 2;
        SENSi = 5 * pow((TEMP - 2000), 2) / 8;

        if (TEMP < -1500) {
            OFFi += 7 * pow((TEMP + 1500), 2);
            SENSi += 4 * pow((TEMP + 1500), 2);
        }
    } else {
        Ti = 2 * pow(dT, 2) / pow(2, 37);
        OFFi = pow((TEMP - 2000), 2) / 16;
        SENSi = 0;
    }

    this->temperature = (TEMP - Ti) / 100.0;
    this->pressure = (digital_pressure_value * (SENS - SENSi) / (1 << 21) - OFF + OFFi) / ((1 << 13) * 10.0);
}

void DepthSensor::ShowDepthSensorWindow() {
    ImGui::Begin("Depth Sensor");

    ImGui::Text("Temperature: %.2f C", this->temperature.load());
    ImGui::Text("Pressure: %.2f mbar", this->pressure.load());

    ImGui::Text("P: %lld", this->P.load());
    ImGui::Text("TEMP: %lld", this->TEMP.load());

    ImGui::End();
}

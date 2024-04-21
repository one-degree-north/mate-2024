//
// Created by Sidharth Maheshwari on 18/4/24.
//
#include <thread>

#include <imgui.h>

#include "depth_sensor.h"
#include "pigpio.h"

DepthSensor::DepthSensor(Pi &pi) : pi(pi) {
    pi.SetGPIOMode(2, PI_ALT0);
    pi.SetGPIOMode(3, PI_ALT0);
    this->pressure_sensor_handle = pi.OpenI2C(1, 0x76);

    pi.WriteI2CByte(this->pressure_sensor_handle, 0x1E); // Reset
    std::this_thread::sleep_for(std::chrono::milliseconds(11));

    for (int i = 0; i < 7; i++) {
        this->CalibrationData[i] = pi.ReadI2CWordData(this->pressure_sensor_handle, 0xA0 + 2 * i);
        this->CalibrationData[i] = ((this->CalibrationData[i] & 0xFF) << 8) | (this->CalibrationData[i] >> 8);
    }


    for (int i = 0; i < 7; i++)
        printf("CalibrationData[%d]: %d\n", i, this->CalibrationData[i]);
}

DepthSensor::~DepthSensor() {
    pi.CloseI2C(this->pressure_sensor_handle);
}

void DepthSensor::PollSensorData() {
    uint32_t digital_pressure_value = 0, digital_temperature_value = 0;
    uint8_t value[3];

    pi.WriteI2CByte(this->pressure_sensor_handle, 0x44); // D1 Pressure conversion
//    std::this_thread::sleep_for(std::chrono::microseconds(2500));
    pi.ReadI2CBlockData(this->pressure_sensor_handle, 0x00, (char*) value, 3);
    digital_pressure_value = (value[0] << 16) | (value[1] << 8) | value[2];

    pi.WriteI2CByte(this->pressure_sensor_handle, 0x54); // D2 Temperature conversion
//    std::this_thread::sleep_for(std::chrono::microseconds(2500));
    pi.ReadI2CBlockData(this->pressure_sensor_handle, 0x00, (char*) value, 3);
    digital_temperature_value = (value[0] << 16) | (value[1] << 8) | value[2];

    int32_t dT = digital_temperature_value - uint32_t(this->CalibrationData[5]) * 256l;
    int32_t TEMP = 2000l + int64_t(dT) * this->CalibrationData[6] / 8388608LL;

    int64_t OFF = int64_t(this->CalibrationData[2]) * 65536l + (int64_t(this->CalibrationData[4]) * dT) / 128l;
    int64_t SENS = int64_t(this->CalibrationData[1]) * 32768l + (int64_t(this->CalibrationData[3]) * dT) / 256l;
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

    this->temperature = (TEMP - Ti) / 100.0;
    this->pressure = (digital_pressure_value * (SENS - SENSi) / 2097152l - OFF + OFFi) / (8192l * 10.0);
}

void DepthSensor::ShowDepthSensorWindow() {
    ImGui::Begin("Depth Sensor");

    ImGui::Text("Temperature: %.2f C", this->temperature.load());
    ImGui::Text("Pressure: %.2f mbar", this->pressure.load());

    ImGui::End();
}

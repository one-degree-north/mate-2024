//
// Created by Sidharth Maheshwari on 18/4/24.
//

#include <thread>

#include "pi.h"

#ifndef MATE_PRESSURE_SENSOR_H
#define MATE_PRESSURE_SENSOR_H


class DepthSensor {
private:
    Pi &pi;
    uint16_t CalibrationData[7] {};
    int pressure_sensor_handle;
    std::atomic<double> temperature = 0, pressure = 0;
    std::atomic_int64_t P, TEMP;
    std::atomic_bool reading_sensor_data = true;
    std::thread data_thread;

    void SensorDataThread();
    void ReadSensorData();
public:
    explicit DepthSensor(Pi &pi);
    ~DepthSensor();

    void ShowDepthSensorWindow();
};


#endif //MATE_PRESSURE_SENSOR_H

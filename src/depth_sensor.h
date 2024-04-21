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

public:
    explicit DepthSensor(Pi &pi);
    ~DepthSensor();

    void ShowDepthSensorWindow();
    void PollSensorData();
};


#endif //MATE_PRESSURE_SENSOR_H

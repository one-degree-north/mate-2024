#include <thread>

#include "pi.h"

#ifndef MATE_DEPTH_SENSOR_H
#define MATE_DEPTH_SENSOR_H


class DepthSensor {
public:
    explicit DepthSensor(Pi &pi);
    ~DepthSensor();

    void ShowDepthSensorWindow();
    double GetDepth() const;
    void PollSensorData();
    void StartDepthSensorThread();
private:
    Pi &pi_;
    uint16_t calibration_data_[7] {};
    int pressure_sensor_handle_;
    std::atomic<double> temperature_ = 0, pressure_ = 0;

    std::atomic_bool depth_sensor_thread_running_ = false;
    std::thread depth_sensor_thread_;
    std::atomic_bool plot_depth=false;
    float depth_graph[10000];
    void DepthSensorThreadLoop();
};


#endif // MATE_DEPTH_SENSOR_H

#include <thread>

#include "pi.h"

#ifndef MATE_ORIENTATION_SENSOR_H
#define MATE_ORIENTATION_SENSOR_H


class OrientationSensor {
public:
    explicit OrientationSensor(Pi &pi);
    ~OrientationSensor();

    void ShowOrientationSensorWindow();
    void StartOrientationSensorThread();

    double GetOrientationYaw() const;
    double GetOrientationRoll() const;
    double GetOrientationPitch() const;

    const uint8_t CALIB_ADDR = 0x35;
    const uint8_t OFFSET_ADDR = 0x55;
    int16_t offset_data[9] = {-157, -182, 93, -1, 1, 0, -29, -36, -34};

private:
    Pi &pi_;
    int orientation_sensor_handle_;
    char actual_calib_data[22];
    std::atomic<uint8_t> calib_data_ = 0, sys_calib_ = 0, gyr_calib_=0, acc_calib_=0, mag_calib_=0;
    std::atomic<double> orientation_yaw_ = 0, orientation_roll_ = 0, orientation_pitch_ = 0;
    float yaw_graph[10000], roll_graph[10000], pitch_graph[10000];
    std::atomic_bool orientation_sensor_thread_running_ = false;
    std::thread orientation_sensor_thread_;
    std::atomic_bool plot_yaw=false, plot_roll=false, plot_pitch=false;
    float moving_avgs[3];
    float max_error = 5;
    uint16_t moving_avg_len = 10;
    void OrientationSensorThreadLoop();
};


#endif //MATE_ORIENTATION_SENSOR_H

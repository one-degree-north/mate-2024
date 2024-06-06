#include <thread>

#include "pi.h"

#ifndef MATE_ORIENTATION_SENSOR_H
#define MATE_ORIENTATION_SENSOR_H


class OrientationSensor {
public:
    explicit OrientationSensor(Pi &pi);
    ~OrientationSensor();

    void ShowOrientationSensorWindow() const;
    void StartOrientationSensorThread();

    double GetOrientationYaw() const;
    double GetOrientationRoll() const;
    double GetOrientationPitch() const;
private:
    Pi &pi_;
    int orientation_sensor_handle_;
    char actual_calib_data[22];
    std::atomic<uint8_t> calib_data_ = 0, sys_calib_ = 0, gyr_calib_=0, acc_calib_=0, mag_calib_=0;
    std::atomic<double> orientation_yaw_ = 0, orientation_roll_ = 0, orientation_pitch_ = 0;
    float orientation_yaw_graph_[10000], orientation_roll_graph_[10000], orientation_pitch_graph_[10000];
    std::atomic_bool orientation_sensor_thread_running_ = false;
    std::thread orientation_sensor_thread_;
    void OrientationSensorThreadLoop();
};


#endif //MATE_ORIENTATION_SENSOR_H

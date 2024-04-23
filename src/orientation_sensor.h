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

    std::atomic<double> orientation_yaw_ = 0, orientation_roll_ = 0, orientation_pitch_ = 0;

    std::atomic_bool orientation_sensor_thread_running_ = false;
    std::thread orientation_sensor_thread_;
    void OrientationSensorThreadLoop();
};


#endif //MATE_ORIENTATION_SENSOR_H

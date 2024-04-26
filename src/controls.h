#include <imgui.h>

#include <thread>

#include "pi.h"
#include "depth_sensor.h"
#include "pid.h"
#include "orientation_sensor.h"

#ifndef MATE_CONTROLS_H
#define MATE_CONTROLS_H


class Controls {
public:
    explicit Controls(Pi &pi);
    ~Controls();

    void ShowControlsWindow();
    void UpdateThrusters(const DepthSensor &depth_sensor, const OrientationSensor &sensor);
    void StartControlsThread(const DepthSensor &depth_sensor, const OrientationSensor &orientation_sensor);
private:
    Pi &pi_;

    bool use_controller_ = false;

    enum Thruster {
        FRONT_RIGHT = 0,
        FRONT_LEFT = 1,
        REAR_RIGHT = 2,
        REAR_LEFT = 3,
        MID_FRONT_RIGHT = 4,
        MID_FRONT_LEFT = 5,
        MID_BACK_RIGHT = 6,
        MID_BACK_LEFT = 7
    };

    std::atomic<double> speed_ = 0.0;
    double target_depth_ = -1.0;

    bool pid_enabled_ = false;

    PID depth_pid_{0.1, 0.1, 0.1, -1.0, 1.0};
    PID roll_pid_{0.1, 0.1, 0.1, -1.0, 1.0};
    PID pitch_pid_{0.1, 0.1, 0.1, -1.0, 1.0};
    PID yaw_pid_{0.1, 0.1, 0.1, -1.0, 1.0};


    struct {
        double forward = 0.0;
        double side = 0.0;
        double up = 0.0;
        double pitch = 0.0;
        double roll = 0.0;
        double yaw = 0.0;
    } movement_vector_;

    std::thread controls_thread_;
    std::atomic_bool controls_thread_running_ = false;
    void ControlsThreadLoop(const DepthSensor &depth_sensor, const OrientationSensor &orientation_sensor);

    static uint16_t DoubleToPulseWidth(double value);
    static void DrawKeyboard();
    static bool BindThrusterKey(double &axis, ImGuiKey positive, ImGuiKey negative);
};


#endif // MATE_CONTROLS_H

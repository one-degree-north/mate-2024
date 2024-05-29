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

    enum Servo{
        ROTATION = 12,
        GRIP = 13
    }
    struct {
        uint16_t rotation = 1500;
        uint16_t grip = 1500;
    } servo_values_;

    enum Thruster {
        FRONT_RIGHT = 3,
        FRONT_LEFT = 4,
        REAR_RIGHT = 0,         // Reversed
        REAR_LEFT = 2,          // Reversed
        MID_FRONT_RIGHT = 7,    // Reversed
        MID_FRONT_LEFT = 6,     // Reversed
        MID_BACK_RIGHT = 1,     // Reversed
        MID_BACK_LEFT = 5       // Reversed
    };

    std::atomic<float> speed_ = 0.0;
    float target_depth_ = -1.0;

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

    static uint32_t DoubleToFeatherWingOnTime(double value);
    static void DrawKeyboard();
    static bool BindThrusterKey(double &axis, ImGuiKey positive, ImGuiKey negative);
    static bool BindServoKey(double &axis, ImGuiKey positive, ImGuiKey negative);
    int feather_wing_handle_;
};


#endif // MATE_CONTROLS_H

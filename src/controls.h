#include <imgui.h>

#include <thread>

#include "pi.h"
#include "depth_sensor.h"

#ifndef MATE_CONTROLS_H
#define MATE_CONTROLS_H


class Controls {
public:
    explicit Controls(Pi &pi);
    ~Controls();

    void ShowControlsWindow();
    void UpdateThrusters(const DepthSensor &depth_sensor);
    void StartControlsThread(const DepthSensor &depth_sensor);
private:
    Pi &pi_;

    enum Thruster {
        FRONT_RIGHT = 0,
        FRONT_LEFT = 1,
        REAR_RIGHT = 2,
        REAR_LEFT = 3,
        MID_LEFT = 4,
        MID_RIGHT = 5
    };

    std::atomic<double> speed_ = 0.0;
    double target_depth_ = -1.0;

    std::atomic<double> k_proportional_ = 2, k_integral_ = 0.5, k_derivative_ = 2;
    std::chrono::time_point<std::chrono::steady_clock> prev_time_;

    struct MovementVector {
        double forward = 0.0;
        double side = 0.0;
        double up = 0.0;
        double pitch = 0.0;
        double roll = 0.0;
        double yaw = 0.0;
    };

    std::atomic<MovementVector> movement_vector_; // from -1 to 1

    std::thread controls_thread_;
    std::atomic_bool controls_thread_running_ = false;
    void ControlsThreadLoop(const DepthSensor &depth_sensor);

    static uint16_t DoubleToPulseWidth(double value);
    static void DrawKeyboard();
    static bool BindThrusterKey(double &axis, ImGuiKey positive, ImGuiKey negative);
};


#endif // MATE_CONTROLS_H

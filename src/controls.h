//
// Created by Sidharth Maheshwari on 18/4/24.
//

#include <imgui.h>

#include "pi.h"

#ifndef MATE_THRUSTER_CONTROL_H
#define MATE_THRUSTER_CONTROL_H


class Controls {
private:
    Pi &pi;

    enum Thruster {
        FRONT_RIGHT = 0,
        FRONT_LEFT = 1,
        REAR_RIGHT = 2,
        REAR_LEFT = 3,
        MID_LEFT = 4,
        MID_RIGHT = 5
    };

    double speed = 0.0;

    struct {
        double forward = 0.0;
        double side = 0.0;
        double up = 0.0;
        double pitch = 0.0;
        double roll = 0.0;
        double yaw = 0.0;
    } movement_vector; // from -1 to 1

    void UpdateThrusters();
    uint16_t DoubleToPulseWidth(double value);
    void DrawKeyboard();
    bool BindThrusterKey(double &axis, ImGuiKey positive, ImGuiKey negative);

public:
    explicit Controls(Pi &pi);
    ~Controls();

    void ShowControlsWindow();
};


#endif //MATE_THRUSTER_CONTROL_H

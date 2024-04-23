#include "pid.h"
#include "imgui.h"

PID::PID(double k_proportional, double k_integral, double k_derivative, double min, double max)
    : k_proportional_(k_proportional),
    k_integral_(k_integral),
    k_derivative_(k_derivative),
    min_(min),
    max_(max),
    prev_time_(std::chrono::steady_clock::now()) {}

double PID::Update(double current_value) {
    double error = target_ - current_value;

    auto current_time = std::chrono::steady_clock::now();
    auto dt = std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - prev_time_).count();
    dt = dt * dt * dt;
    prev_time_ = current_time;

    integral_ += error * dt;

    double p = k_proportional_ * error;
    double i = k_integral_ * integral_;
    double d = k_derivative_ * error / dt;

    return p + i + d;
}

double PID::GetTarget() const {
    return target_;
}

void PID::SetTarget(double target) {
    target_ = target;
}

void PID::DrawPIDConfigWindow() {
    ImGui::SliderFloat("P", reinterpret_cast<float *>(&k_proportional_), 0.0, 10.0);
    ImGui::SliderFloat("I", reinterpret_cast<float *>(&k_integral_), 0.0, 10.0);
    ImGui::SliderFloat("D", reinterpret_cast<float *>(&k_derivative_), 0.0, 10.0);
    ImGui::SliderFloat("Max", reinterpret_cast<float *>(&max_), 0.0, 360.0);
    ImGui::SliderFloat("Min", reinterpret_cast<float *>(&min_), 0.0, 360.0);

    if (ImGui::Button("Reset")) {
        Reset();
    }
}

void PID::Reset() {
    integral_ = 0.0;
    prev_time_ = std::chrono::steady_clock::now();
}

PID::~PID() = default;



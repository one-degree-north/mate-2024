#include "pid.h"
#include "imgui.h"

PID::PID(double k_proportional, double k_integral, double k_derivative, double min, double max)
    : k_proportional_(k_proportional),
    k_integral_(k_integral),
    k_derivative_(k_derivative),
    max_(max),
    min_(min),
    prev_time_(std::chrono::steady_clock::now()) {}

double PID::Update(double current_value) {
    double error = target_ - current_value;
    last_error_ = error;

    auto current_time = std::chrono::steady_clock::now();
    double dt = std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - prev_time_).count() / 1.0e9;
    prev_time_ = current_time;

    integral_ += error * dt;

    double p = k_proportional_ * error;
    last_p_ = p;
    double i = k_integral_ * integral_;
    last_i_ = i;
    double d = k_derivative_ * error / dt;
    last_d_ = d;

    last_val_ = std::clamp(p + i + d, min_.load(), max_.load());
    return last_val_;
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
    ImGui::SliderFloat("Max", reinterpret_cast<float *>(&max_), -1, 1);
    ImGui::SliderFloat("Min", reinterpret_cast<float *>(&min_), -1, 1);
    ImGui::Text("PID Output: %f", last_val_.load());
    ImGui::Text("PID Error: %f", last_error_.load());
    ImGui::Text("P: %f", last_p_.load());
    ImGui::Text("I: %f", last_i_.load());
    ImGui::Text("D: %f", last_d_.load());
    ImGui::Text("Set point: %f", target_.load());

    if (ImGui::Button("Reset")) {
        Reset();
    }
}

void PID::Reset() {
    integral_ = 0.0;
    prev_time_ = std::chrono::steady_clock::now();
}

PID::~PID() = default;



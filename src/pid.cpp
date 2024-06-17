#include "pid.h"
#include "imgui.h"

PID::PID(float k_proportional, float k_integral, float k_derivative, float min, float max, bool eul)
    : k_proportional_(k_proportional),
    k_integral_(k_integral),
    k_derivative_(k_derivative),
    max_(max),
    min_(min),
    eul_(eul),
    prev_time_(std::chrono::steady_clock::now()) {}

double PID::Update(double current_value) {
    float error = 0;
    if (eul_){
        // float rel_curr = current_value - target_;
        error = target_ - current_value;
        if (error > 180){
            error = -1*(error-360);
        }
        if (error < -180){
            error = (error+360);
        }
        // error = fmod(error, 360);
        // error *= -1;
        // error = fmod(error+180.0, 360.0)-180.0;
    }
    else{
        error = target_ - current_value;
    }

    last_error_ = error;

    auto current_time = std::chrono::steady_clock::now();
    double dt = std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - prev_time_).count() / 1.0e9;

    prev_time_ = current_time;

    integral_ += error * dt;

    float p = k_proportional_ * error;
    last_p_ = p;
    float i = k_integral_ * integral_;
    last_i_ = i;
    float d = k_derivative_ * error / dt;
    last_d_ = d;

    // float linear_val = p+i+d;   // thruster output force is ~proportional to speed^3
    // linear_val = linear_val * linear_val * linear_val;
    last_val_ = std::clamp(p+i+d, min_.load(), max_.load());
    return last_val_;
}

double PID::GetTarget() const {
    return target_;
}

void PID::SetTarget(double target) {
    target_ = target;
}

void PID::DrawPIDConfigWindow() {
    ImGui::SliderFloat("P", reinterpret_cast<float *>(&this->k_proportional_), 0.0, 10.0);
    ImGui::SliderFloat("I", reinterpret_cast<float *>(&this->k_integral_), 0.0, 10.0);
    ImGui::SliderFloat("D", reinterpret_cast<float *>(&this->k_derivative_), 0.0, 10.0);

    ImGui::Text("PID Output: %f", last_val_.load());
    ImGui::Text("PID Error: %f", last_error_.load());
    ImGui::Text("P: %f", last_p_.load());
    ImGui::Text("I: %f", last_i_.load());
    ImGui::Text("D: %f", last_d_.load());
    ImGui::Text("P: %f", k_proportional_.load());
    ImGui::Text("I: %f", k_integral_.load());
    ImGui::Text("D: %f", k_derivative_.load());
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



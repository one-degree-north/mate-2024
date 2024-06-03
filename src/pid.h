#include <chrono>

#ifndef MATE_PID_H
#define MATE_PID_H

class PID {
public:
    PID(double k_proportional, double k_integral, double k_derivative, double min, double max);
    ~PID();

    double Update(double current_value);

    double GetTarget() const;
    void SetTarget(double target);

    void DrawPIDConfigWindow();

    void Reset();
private:
    std::atomic<double> k_proportional_, k_integral_, k_derivative_, max_, min_, target_ = 0.0;
    double integral_ = 0.0;
    std::atomic<double> last_val_, last_error_, last_p_, last_i_, last_d_;
    std::chrono::time_point<std::chrono::steady_clock> prev_time_;
};


#endif //MATE_PID_H

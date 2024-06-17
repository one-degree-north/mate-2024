from enum import Enum
import time

class PID():
    def __init__(self, k_const=0, i_const=0, d_const=0, eul=False):
        self.max = 10
        self.min = -10
        self.target = 0
        self.eul = eul
        self.integral = 0
        self.prev_time = time.process_time_ns()
        self.k_const = k_const
        self.i_const = i_const
        self.d_const = d_const

    def start_pid(self):
        self.prev_time = time.process_time_ns()

    def set_target(self, target):
        self.target = target
    
    def update(self, current_value):
        error = 0
        if (self.eul):
            rel_curr = current_value - self.target
            error = self.target - current_value
            if (error > 180):
                error = -1*(error-360)
            
            if (error < -180):
                error = (error+360)
        else:
            error = self.target - current_value
        last_error_ = error

        current_time = time.process_time_ns()
        dt = current_time - self.prev_time
        self.prev_time = current_time

        self.integral += error * dt

        p = self.k_const * error

        i = self.i_const * self.integral

        d = self.d_const * error / dt

        final_val = p+i+d

        # adjust for final_val being power 3, later maybe

        if (final_val > self.max):
            final_val = self.max
        
        if (final_val < self.min):
            final_val = self.min

        return final_val

class Controls():
    def __init__(self, sensors=None):
        #     enum Thruster {
    #     FRONT_RIGHT = 3,
    #     FRONT_LEFT = 4,
    #     REAR_RIGHT = 0,         // Reversed
    #     REAR_LEFT = 2,          // Reversed
    #     MID_FRONT_RIGHT = 7,    // Reversed
    #     MID_FRONT_LEFT = 6,     // Reversed
    #     MID_BACK_RIGHT = 1,     // Reversed
    #     MID_BACK_LEFT = 5       // Reversed
    # };
        self.thrusters = Enum("t_pin", ["REAR_RIGHT", "MID_BACK_RIGHT", "REAR_LEFT", "FRONT_RIGHT", "FRONT_LEFT", "MID_BACK_LEFT", "MID_FRONT_LEFT", "MID_FRONT_RIGHT"])
        self.thrust_values = {"front_left": 0, "front_right": 0, "back_left": 0, "back_right": 0, "mid_front_left":0, "mid_front_right":0, "mid_back_left":0, "mi_back_right":0}
        self.speed = 0
        self.movements = {"front": 0, "side": 0, "up":0, "yaw":0, "roll":0, "pitch":0}
        self.pid_enabled = False
        self.depth_pid = PID()
        self.yaw_pid = PID()
        self.roll_pid = PID()
        self.pitch_pid = PID()
        self.sensors = sensors

    def update_thrusters(self):
        if (self.pid_enabled):
            self.movements.up = self.depth_pid.update(self.sensors.data["depth"])
            self.movements.yaw = self.yaw_pid.update(self.sensors.data["yaw"])
            self.movements.roll = self.roll_pid.update(self.sensors.data["roll"])
            self.movements.pitch = self.pitch_pid.update(self.sensors.data["pitch"])

            self.thrust_values["front_left"] = (self.speed * (self.movements.forward + self.movements.side) + self.movements.yaw) / 30.0;
            self.thrust_values["front_right"] = (self.speed * (self.movements.forward - self.movements.side) - self.movements.yaw) / 30.0;
            self.thrust_values["back_left"] = -(self.speed * (self.movements.forward - self.movements.side) + self.movements.yaw) / 30.0;
            self.thrust_values["back_right"] = -(self.speed * (self.movements.forward + self.movements.side) - self.movements.yaw) / 30.0;
            self.thrust_values["mid_front_left"] = -(self.movements.up - self.movements.roll + self.movements.pitch) / 30.0;
            self.thrust_values["mid_front_right"] = -(self.movements.up + self.movements.roll + self.movements.pitch) / 30.0;
            self.thrust_values["mid_back_left"] = -(self.movements.up - self.movements.roll - self.movements.pitch) / 30.0;
            self.thrust_values["mid_back_right"] = -(self.movements.up + self.movements.roll - self.movements.pitch) / 30.0;

        else:
            self.thrust_values["front_left"] = self.speed * (self.movements.forward + self.movements.side + self.movements.yaw) / 30.0;
            self.thrust_values["front_right"] = self.speed * (self.movements.forward - self.movements.side - self.movements.yaw) / 30.0;
            self.thrust_values["back_left"] = -self.speed * (self.movements.forward - self.movements.side + self.movements.yaw) / 30.0;
            self.thrust_values["back_right"] = -self.speed * (self.movements.forward + self.movements.side - self.movements.yaw) / 30.0;
            self.thrust_values["mid_front_left"] = -self.speed * (self.movements.up - self.movements.roll + self.movements.pitch) / 30.0;
            self.thrust_values["mid_front_right"] = -self.speed * (self.movements.up + self.movements.roll + self.movements.pitch) / 30.0;
            self.thrust_values["mid_back_left"] = -self.speed * (self.movements.up - self.movements.roll - self.movements.pitch) / 30.0;
            self.thrust_values["mid_back_right"] = -self.speed * (self.movements.up + self.movements.roll - self.movements.pitch) / 30.0;


void Controls::UpdateThrusters(const DepthSensor &depth_sensor, const OrientationSensor &orientation_sensor) {
    double front_left;
    double front_right;
    double back_left;
    double back_right;
    double mid_front_left;
    double mid_front_right;
    double mid_back_left;
    double mid_back_right;
    if (pid_enabled_) {
        self.movements.up = depth_pid_.Update(depth_sensor.GetDepth());
        self.movements.yaw = yaw_pid_.Update(orientation_sensor.GetOrientationYaw());
        self.movements.roll = roll_pid_.Update(orientation_sensor.GetOrientationRoll());
        self.movements.pitch = pitch_pid_.Update(orientation_sensor.GetOrientationPitch());

        front_left = (speed_ * (self.movements.forward + self.movements.side) + self.movements.yaw) / 30.0;
        front_right = (speed_ * (self.movements.forward - self.movements.side) - self.movements.yaw) / 30.0;
        back_left = -(speed_ * (self.movements.forward - self.movements.side) + self.movements.yaw) / 30.0;
        back_right = -(speed_ * (self.movements.forward + self.movements.side) - self.movements.yaw) / 30.0;
        mid_front_left = -(self.movements.up - self.movements.roll + self.movements.pitch) / 30.0;
        mid_front_right = -(self.movements.up + self.movements.roll + self.movements.pitch) / 30.0;
        mid_back_left = -(self.movements.up - self.movements.roll - self.movements.pitch) / 30.0;
        mid_back_right = -(self.movements.up + self.movements.roll - self.movements.pitch) / 30.0;
    }
    else{
        front_left = speed_ * (self.movements.forward + self.movements.side + self.movements.yaw) / 30.0;
        front_right = speed_ * (self.movements.forward - self.movements.side - self.movements.yaw) / 30.0;
        back_left = -speed_ * (self.movements.forward - self.movements.side + self.movements.yaw) / 30.0;
        back_right = -speed_ * (self.movements.forward + self.movements.side - self.movements.yaw) / 30.0;
        mid_front_left = -speed_ * (self.movements.up - self.movements.roll + self.movements.pitch) / 30.0;
        mid_front_right = -speed_ * (self.movements.up + self.movements.roll + self.movements.pitch) / 30.0;
        mid_back_left = -speed_ * (self.movements.up - self.movements.roll - self.movements.pitch) / 30.0;
        mid_back_right = -speed_ * (self.movements.up + self.movements.roll - self.movements.pitch) / 30.0;
    }
    union {
        uint32_t thrusts[8];
        char feather_wing_register_data[32];
    } data {};


    data.thrusts[Thruster::FRONT_LEFT] = Controls::DoubleToFeatherWingOnTime(front_left);
    data.thrusts[Thruster::FRONT_RIGHT] = Controls::DoubleToFeatherWingOnTime(front_right);
    data.thrusts[Thruster::REAR_LEFT] = Controls::DoubleToFeatherWingOnTime(back_left);
    data.thrusts[Thruster::REAR_RIGHT] = Controls::DoubleToFeatherWingOnTime(back_right);
    data.thrusts[Thruster::MID_FRONT_LEFT] = Controls::DoubleToFeatherWingOnTime(mid_front_left);
    data.thrusts[Thruster::MID_FRONT_RIGHT] = Controls::DoubleToFeatherWingOnTime(mid_front_right);
    data.thrusts[Thruster::MID_BACK_LEFT] = Controls::DoubleToFeatherWingOnTime(mid_back_left);
    data.thrusts[Thruster::MID_BACK_RIGHT] = Controls::DoubleToFeatherWingOnTime(mid_back_right);

    for (uint32_t &thrust : data.thrusts)
        thrust <<= 16;


//    int res = pi_.WriteI2CBlockData(feather_wing_handle_, 0x06, data.feather_wing_data, 32);
//    Pi::I2CZipCommand cmd;
    for (int i = 0; i < 32; i++)
//        cmd.WriteByte(0x06 + i, data.feather_wing_register_data[i]);
//
//    int a = cmd.Send(pi_, feather_wing_handle_);
//    if (a != 0) std::cout << a << std::endl;

        pi_.WriteI2CByteData(feather_wing_handle_, 0x06 + i, data.feather_wing_register_data[i]);

    pi_.SetServoPulseWidth(12, std::clamp((int) ((claw_rotation_ + 1) * 1000 + 500),  500, 2500));
    pi_.SetServoPulseWidth(13, std::clamp((int) ((claw_open_ + 1) * 1000 + 500),  500, 2500));

//    pi_.SetServoPulseWidth(Thruster::MID_FRONT_LEFT, Controls::DoubleToPulseWidth(mid_front_left));
//    pi_.SetServoPulseWidth(Thruster::MID_FRONT_RIGHT, Controls::DoubleToPulseWidth(mid_front_right));
//    pi_.SetServoPulseWidth(Thruster::MID_BACK_LEFT, Controls::DoubleToPulseWidth(mid_back_left));
//    pi_.SetServoPulseWidth(Thruster::MID_BACK_RIGHT, Controls::DoubleToPulseWidth(mid_back_right));
//    pi_.SetServoPulseWidth(Thruster::FRONT_LEFT, Controls::DoubleToPulseWidth(front_left));
//    pi_.SetServoPulseWidth(Thruster::FRONT_RIGHT, Controls::DoubleToPulseWidth(front_right));
//    pi_.SetServoPulseWidth(Thruster::REAR_LEFT, Controls::DoubleToPulseWidth(back_left));
//    pi_.SetServoPulseWidth(Thruster::REAR_RIGHT, Controls::DoubleToPulseWidth(back_right));
}

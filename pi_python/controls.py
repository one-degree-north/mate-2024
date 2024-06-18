from enum import Enum
import time
from adafruit_pca9685 import PCA9685
import threading
import board
import pigpio

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
    def __init__(self, sensors=None, debug=True, server=None, send_data=True):
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
        self.pca = PCA9685(i2c_bus=board.I2C(),address=0x40, reference_clock_speed=25800000)
        self.pca.frequency=50
        self.thrusters = Enum("t_pin", ["REAR_RIGHT", "MID_BACK_RIGHT", "REAR_LEFT", "FRONT_RIGHT", "FRONT_LEFT", "MID_BACK_LEFT", "MID_FRONT_LEFT", "MID_FRONT_RIGHT"])
        self.thrust_values = [0,0,0,0,0,0,0,0]
        self.speed = 0
        self.movements = {"forward": 0, "side": 0, "up":0, "yaw":0, "roll":0, "pitch":0}
        self.pid_enabled = False
        self.depth_pid = PID()
        self.yaw_pid = PID()
        self.roll_pid = PID()
        self.pitch_pid = PID()
        self.sensors = sensors
        self.loop_lock = threading.Lock()
        self.delay = 0.01     # in seconds
        self.pig = pigpio.pi()
        self.claw_rot_pin = 12
        self.claw_grip_pin = 13
        self.debug = debug
        self.server = server
        self.send_data = send_data

    def thrust_to_clock(self, t):
        c = int(0xFFFF * (0.025 * t + 0.075))
        if c > int(float(0xFFFF) * (0.125)):
            c = int(float(0xFFFF) * (0.125))
        if c < int(float(0xFFFF) * (0.025)):
            c = int(float(0xFFFF) * (0.025))
        return c

    def start_loop(self):
        thread = threading.Thread(target=self.update_loop)
        thread.start()

    def update_loop(self):
        while True:
            self.loop_lock.acquire()
            self.update_thrusters()
            self.loop_lock.release()
            time.sleep(self.delay)

    def update_thrusters(self):
        self.sensors.read_sensors()

        if (self.pid_enabled):
            self.movements["up"] = self.depth_pid.update(self.sensors.data["depth"])
            self.movements["yaw"] = self.yaw_pid.update(self.sensors.data["yaw"])
            self.movements["roll"] = self.roll_pid.update(self.sensors.data["roll"])
            self.movements["pitch"] = self.pitch_pid.update(self.sensors.data["pitch"])

            self.thrust_values[self.thrusters.FRONT_LEFT.value-1] = (self.speed * (self.movements["forward"] + self.movements["side"]) + self.movements["yaw"]) / 30.0
            self.thrust_values[self.thrusters.FRONT_RIGHT.value-1] = (self.speed * (self.movements["forward"] - self.movements["side"]) - self.movements["yaw"]) / 30.0
            self.thrust_values[self.thrusters.REAR_LEFT.value-1] = -(self.speed * (self.movements["forward"] - self.movements["side"]) + self.movements["yaw"]) / 30.0
            self.thrust_values[self.thrusters.REAR_RIGHT.value-1] = -(self.speed * (self.movements["forward"] + self.movements["side"]) - self.movements["yaw"]) / 30.0
            self.thrust_values[self.thrusters.MID_FRONT_LEFT.value-1] = -(self.movements["up"] - self.movements["roll"] + self.movements["pitch"]) / 30.0
            self.thrust_values[self.thrusters.MID_FRONT_RIGHT.value-1] = -(self.movements["up"] + self.movements["roll"] + self.movements["pitch"]) / 30.0
            self.thrust_values[self.thrusters.MID_BACK_LEFT.value-1] = -(self.movements["up"] - self.movements["roll"] - self.movements["pitch"]) / 30.0
            self.thrust_values[self.thrusters.MID_BACK_RIGHT.value-1] = -(self.movements["up"] + self.movements["roll"] - self.movements["pitch"]) / 30.0

        else:
            self.thrust_values[self.thrusters.FRONT_LEFT.value-1] = self.speed * (self.movements["forward"] + self.movements["side"] + self.movements["yaw"]) / 30.0
            self.thrust_values[self.thrusters.FRONT_RIGHT.value-1] = self.speed * (self.movements["forward"] - self.movements["side"] - self.movements["yaw"]) / 30.0
            self.thrust_values[self.thrusters.REAR_LEFT.value-1] = -self.speed * (self.movements["forward"] - self.movements["side"] + self.movements["yaw"]) / 30.0
            self.thrust_values[self.thrusters.REAR_RIGHT.value-1] = -self.speed * (self.movements["forward"] + self.movements["side"] - self.movements["yaw"]) / 30.0
            self.thrust_values[self.thrusters.MID_FRONT_LEFT.value-1] = -self.speed * (self.movements["up"] - self.movements["roll"] + self.movements["pitch"]) / 30.0
            self.thrust_values[self.thrusters.MID_FRONT_RIGHT.value-1] = -self.speed * (self.movements["up"] + self.movements["roll"] + self.movements["pitch"]) / 30.0
            self.thrust_values[self.thrusters.MID_BACK_LEFT.value-1] = -self.speed * (self.movements["up"] - self.movements["roll"] - self.movements["pitch"]) / 30.0
            self.thrust_values[self.thrusters.MID_BACK_RIGHT.value-1] = -self.speed * (self.movements["up"] + self.movements["roll"] - self.movements["pitch"]) / 30.0
        if self.debug:
            print(f"{self.thrust_values}, {self.movements}, {self.speed}")
        for i in range(8):
            #print(self.thrust_values[i]/30 * 2000)
            self.pca.channels[i].duty_cycle = self.thrust_to_clock(self.thrust_values[i])
        if self.send_data:
            if self.debug:
                print(f"trying to send sens")
            self.server.send_sens_data(self.sensors.data)
            if self.debug:
                print(f"funny send sens")

    def set_manual_thrust(self, front, side, up, yaw, pitch, roll, speed):
        self.loop_lock.acquire()
        self.movements["forward"] = front
        self.movements["side"] = side
        self.movements["up"] = up
        self.movements["yaw"] = yaw
        self.movements["pitch"] = pitch
        self.movements["roll"] = roll
        self.speed = speed
        self.loop_lock.release()
    def set_pid_thrust(self, front, side, up, yaw, pitch, roll, speed):
        self.loop_lock.acquire()
        self.movements["forward"] = front
        self.movements["side"] = side
        self.depth_pid.set_target(up)
        self.yaw_pid.set_target(yaw)
        self.pitch_pid.set_target(pitch)
        self.roll_pid.set_target(roll)
        self.speed = speed
        self.loop_lock.release()

    def set_grip(self, micro):
        self.pig.set_servo_pulsewidth(self.claw_grip_pin, micro[0])

    def set_rot(self, micro):
        self.pig.set_servo_pulsewidth(self.claw_rot_pin, micro[0])

if __name__ == "__main__":
    pass

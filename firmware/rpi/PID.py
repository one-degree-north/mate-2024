import time
import threading
from data import OpiDataProcess

class PIDController:
    def __init__(self, p_const=1, i_const=1, d_const=1):
        self.p_const = p_const
        self.i_const = i_const
        self.d_const = d_const
        self.integral = 0
        self.prev_error = 0

    def calculate_output(self, error, delta_time):
        self.integral += error * delta_time
        derivative = (error - self.prev_error) / delta_time
        output = self.p_const * error + self.i_const * self.integral + self.d_const * derivative
        self.prev_error = error
        return output

class ThrusterState:
    def on_tick(self):
        raise NotImplementedError

    def set_target(self, target):
        raise NotImplementedError

class ManualThrusterState(ThrusterState):
    def __init__(self, manual_input):
        self.manual_input = manual_input

    def on_tick(self):
        return self.manual_input

    def set_target(self, manual_input):
        self.manual_input = manual_input

class PIDThrusterState(ThrusterState):
    def __init__(self, target, opi_data, delta_time):
        self.target = target
        self.opi_data = opi_data
        self.pid_controllers = [PIDController() for _ in range(3)]
        self.delta_time = delta_time

    def on_tick(self):
        return [pid.calculate_output(t - o, self.delta_time) for pid, t, o in zip(self.pid_controllers, self.target, self.opi_data.vel)]

    def set_target(self, target):
        self.target = target

class HoldThrusterState(ThrusterState):
    def __init__(self, opi_data, delta_time):
        self.target = [0, 0, 0]
        self.opi_data = opi_data
        self.pid_controllers = [PIDController() for _ in range(3)]
        self.delta_time = delta_time

    def on_tick(self):
        return [pid.calculate_output(t - o, self.delta_time) for pid, t, o in zip(self.pid_controllers, self.target, self.opi_data.vel)]

    def set_target(self, target):
        pass

class DriftThrusterState(ThrusterState):
    def on_tick(self):
        return [0, 0, 0]

    def set_target(self, target):
        pass

class ThrusterController:
    def __init__(self, move_delta_time=0.05, stop_event=None, use_stop_event=False, debug=False, passthrough=False):
        self.data = None
        self.pos_state = DriftThrusterState()
        self.rot_state = DriftThrusterState()
        self.move_delta_time = move_delta_time
        self.mcu_interface = None
        self.max_thrust = 0.3
        self.passthrough = passthrough

        self.stop_event = stop_event
        self.use_stop_event = use_stop_event
        self.debug = debug

        self.thruster_order = [6, 2, 1, 4, 5, 3, 0, 7]
        self.reversed_thrust = [False, True, True, True, False, True, True, True]

    def set_interface(self, mcu_interface):
        self.mcu_interface = mcu_interface

    def set_data(self, opi_data):
        self.data = opi_data

    def start_controller_loop(self):
        move_thread = threading.Thread(target=self._controller_loop)
        move_thread.start()

    def _controller_loop(self):
        while not (self.use_stop_event and self.stop_event.is_set()) and not self.passthrough:
            pos_thrust = self.pos_state.on_tick()
            rot_thrust = self.rot_state.on_tick()
            total_thrust = self._calculate_total_thrust(pos_thrust, rot_thrust)

            if self.debug:
                print(f"Writing thrust: {total_thrust}")
                print(f"Elapsed: {(time.time() - t) * 1000.0:.4f}ms")
                t = time.time()

            self.mcu_interface.set_thrusters(total_thrust)
            time.sleep(self.move_delta_time)

    def _calculate_total_thrust(self, pos_thrust, rot_thrust):
        mov = self._calculate_movement(pos_thrust, rot_thrust)
        total_thrust = [0] * 8

        for i in range(4):
            total_thrust[self.thruster_order[i]] = mov.f + (-1) ** i * mov.s + (-1) ** (i // 2) * mov.y

        for i in range(4, 8):
            total_thrust[self.thruster_order[i]] = mov.u + (-1) ** (i - 4) * mov.p + (-1) ** (i % 2) * mov.r

        max_thrust = max(map(abs, total_thrust))

        if max_thrust > self.max_thrust:
            total_thrust = [t / max_thrust * self.max_thrust for t in total_thrust]

        for i in range(8):
            total_thrust[i] = int(1500 + (-1 if self.reversed_thrust[i] else 1) * 500 * total_thrust[i])
            total_thrust[i] = max(1500 - 500 * self.max_thrust, min(total_thrust[i], 1500 + 500 * self.max_thrust))

        return total_thrust

    def set_raw_thrust(self, thrusts):
        pass

    def set_pos_manual(self, moves):
        self._set_state("pos_manual", moves)

    def set_pos_target_vel(self, vels):
        self._set_state("pos_pid", vels)

    def set_pos_hold(self):
        self._set_state("pos_hold")

    def set_pos_drift(self):
        self._set_state("pos_drift")

    def set_rot_manual(self, thrusts):
        self._set_state("rot_manual", thrusts)

    def set_rot_vel(self, vels):
        self._set_state("rot_gyro", vels)

    def set_rot_angle(self, angles):
        self._set_state("rot_angle", angles)

    def set_rot_hold(self):
        self._set_state("rot_hold")

    def set_rot_drift(self):
        self._set_state("rot_drift")

    def _set_state(self, move_type, target=None):
        pos_state_map = {
            "pos_manual": ManualThrusterState(target),
            "pos_pid": PIDThrusterState(target, self.data.data, self.move_delta_time),
            "pos_hold": HoldThrusterState(self.data.data, self.move_delta_time),
            "pos_drift": DriftThrusterState()
        }

        rot_state_map = {
            "rot_manual": ManualThrusterState(target),
            "rot_gyro": PIDThrusterState(target, self.data.data, self.move_delta_time),
            "rot_angle": PIDThrusterState(target, self.data.data, self.move_delta_time),
            "rot_hold": HoldThrusterState(self.data.data, self.move_delta_time),
            "rot_drift": DriftThrusterState()
        }

        self.pos_state = pos_state_map.get(move_type, self.pos_state)
        self.rot_state = rot_state_map.get(move_type, self.rot_state)

if __name__ == "__main__":
    thruster_controller = ThrusterController()
    data = OpiDataProcess()
    thruster_controller.set_data(data)
    data.start_bno_reading()
    thruster_controller.start_controller_loop()

    while True:
        val = input("pos / rot > ")
        move_type = input("type > ")
        target = input("thrusts > ").split()
        thrusts = [float(tar) for tar in target]

        if val == "pos":
            thruster_controller.set_pos_state(move_type, thrusts)
        elif val == "rot":
            thruster_controller.set_rot_state(move_type, thrusts)

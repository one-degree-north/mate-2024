import dearpygui.dearpygui as dpg
import socket, select, queue, threading, struct
from collections import namedtuple
from dataclasses import dataclass
import cv2
import sys


class PIClient:
    #code to communicate to opi
    move = namedtuple("move", ['f', 's', 'u', 'p', 'r', 'y'])
    def __init__(self, server_address=("192.168.2.2", 7774)):
        self.out_queue = queue.Queue()
        self.client_thread = threading.Thread(target=self.client_loop, args=[server_address], daemon=True)
        self.client_thread.start()
        self.data = {"sys_calib": 0,
                    "gyro_calib":0,
                    "accel_calib":0,
                    "mag_calib":0,
                    "roll":0,
                    "pitch":0,
                    "yaw":0,
                    "depth":0,
                    "temp":0
                    }
        self.data_length = 1000
        self.past_data_roll = [0] * self.data_length
        self.past_data_pitch = [0] * self.data_length
        self.past_data_yaw = [0] * self.data_length
        self.past_data_depth = [0] * self.data_length
    
    def client_loop(self, server_address):
        while True:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            # self.sock.connect(address)
            self.sock.sendto(bytes([0x00]), server_address) #try to connect
            print("client sent attempt to connect")
            while True:
                r, w, x = select.select([self.sock], [self.sock], [self.sock])
                for sock in r:  #ready to read!
                    print("received data")
                    self.process_data(sock.recv(2048))
                
                for sock in w:  #ready to write!
                    if not self.out_queue.empty():
                        sock.sendto(self.out_queue.get(), server_address)
                        print("wrote data")
                
                for sock in x:  #exception 8^(. Create new socket and try to connect again.
                    print("exception in networking")
    
    def process_data(self, data):
        sensor_data = struct.unpack("!cBBBBfffff", data)
        print(f"sensor_data: {sensor_data}")
        item_index = 0
        for x, y in self.data.items():
            self.data[x] = sensor_data[item_index]
            item_index += 1
        for i in range(self.data_length-1):
            self.past_data_roll[i+1] = self.past_data_roll[i]
            self.past_data_pitch[i+1] = self.past_data_pitch[i]
            self.past_data_yaw[i+1] = self.past_data_yaw[i]
            self.past_data_depth[i+1] = self.past_data_depth[i]
        self.past_data_roll[i] = self.data["roll"]
        self.past_data_pitch[i] = self.data["pitch"]
        self.past_data_yaw[i] = self.data["yaw"]
        self.past_data_depth[i] = self.data["depth"]

    def move_servo(self, pulse1, pulse2):
        self.out_queue.put(struct.pack("!cHH", bytes([0x20]), int(pulse1//2), int(pulse2//2)))

    def set_grip(self, micro):
        self.out_queue.put(struct.pack("!cI", bytes([0x30]), micro))

    def set_rot(self, micro):
        self.out_queue.put(struct.pack("!cI", bytes([0x31]), micro))

    def set_manual_thrust(self, moves, speed):
        self.out_queue.put(struct.pack("!cfffffff", bytes([0x20]), *(moves[0:6]), speed))

    def set_pid_target(self, moves, target_depth, target_yaw, target_pitch, target_roll, speed):    # moves are in target m/s
        self.out_queue.put(struct.pack("!cfffffff", bytes([0x21]), *(moves[0:2]), target_depth, target_yaw, target_pitch, target_roll, speed))
 
    def set_pid_constant(self, constant, value):
        data_target = bytes([0x00]) #0: depth, 1: yaw, 2: pitch, 3: roll
        data_type = bytes([0x00]) #0: p, 1: i, 2: d
        if constant == "yaw_p":
            data_target = bytes([0x02])
            data_type = bytes([0x00])
        if constant == "yaw_i":
            data_target = bytes([0x02])
            data_type = bytes([0x01])
        if constant == "yaw_d":
            data_target = bytes([0x02])
            data_type = bytes([0x02])
        self.out_queue.put(struct.pack("!cccf", bytes([0x040]), data_target, data_type, value))

    def test_connection(self):
        self.out_queue.put(struct.pack("!c", bytes([0x10])))
    
    def reset_all_pid(self):
        self.out_queue.put(struct.pack("!c", bytes([0x41])))

if __name__ == "__main__":
    client = PIClient()
    debug = True
    pid_enabled = False
    changed = False
    movement_vector = [0, 0, 0, 0, 0, 0]
    target_yaw = 0
    target_roll = 0
    target_depth = 0
    target_pitch = 0
    speed = 0
    claw_rot = 1500
    claw_grip = 1500
    
    def pid_en_callback(sender, app_data):
        global pid_enabled
        pid_enabled= app_data

    def reset_all_pid(sender, app_data):
        client.reset_all_pid()

    def set_rot(sender, app_data):
        client.set_rot(int(app_data))
    
    def set_grip(sender, app_data):
        client.set_grip(int(app_data))

    def set_speed(sender, app_data):
        global changed
        changed = True
        global speed
        speed = app_data

    def set_pid_constant(sender, app_data, user_data):
        client.set_pid_constant(user_data, app_data)

    dpg.create_context()

    def handle_keyboard_press(sender, app_data):
        global movement_vector
        global target_roll
        global target_pitch
        global target_yaw
        global pid_enabled
        global client
        global speed
        global debug
        if debug:
            print(f"key: {app_data}")
        match(app_data):
            case dpg.mvKey_W:
                movement_vector[0] = 1
            case dpg.mvKey_S:
                movement_vector[0] = -1
            case dpg.mvKey_D:
                movement_vector[1] = 1
            case dpg.mvKey_A:
                movement_vector[1] = -1
            case dpg.mvKey_Shift:
                movement_vector[2] = 1
            case dpg.mvKey_Spacebar:
                movement_vector[2] = -1
        if pid_enabled:
            match(app_data):
                case dpg.mvKey_J:
                    target_roll -= 5
                case dpg.mvKey_L:
                    target_roll += 5
                case dpg.mvKey_I:
                    target_pitch += 5
                case dpg.mvKey_K:
                    target_pitch += -5
                case dpg.mvKey_Q:
                    target_yaw += -5
                case dpg.mvKey_E:
                    target_yaw += 5
        else:
            match(app_data):
                case dpg.mvKey_J:
                    movement_vector[5] = -1
                case dpg.mvKey_L:
                    movement_vector[5] = 1
                case dpg.mvKey_I:
                    movement_vector[4] = 1
                case dpg.mvKey_K:
                    movement_vector[4] = -1
                case dpg.mvKey_Q:
                    movement_vector[3] = -1
                case dpg.mvKey_E:
                    movement_vector[3] = 1
        if pid_enabled:
            client.set_pid_target(movement_vector[0:2], target_depth, target_yaw, target_pitch, target_roll, speed)
        else:
            client.set_manual_thrust(movement_vector, speed)

    def handle_keyboard_release(sender, app_data):
        global movement_vector
        global target_roll
        global target_pitch
        global target_yaw
        global pid_enabled
        global client
        global speed
        global debug
        if debug:
            print(f"key: {app_data}")
        match(app_data):
            case dpg.mvKey_W:
                movement_vector[0] = 0
            case dpg.mvKey_S:
                movement_vector[0] = 0
            case dpg.mvKey_D:
                movement_vector[1] = 0
            case dpg.mvKey_A:
                movement_vector[1] = 0
            case dpg.mvKey_Shift:
                movement_vector[2] = 0
            case dpg.mvKey_Spacebar:
                movement_vector[2] = 0
        if not pid_enabled:
            match(app_data):
                case dpg.mvKey_J:
                    movement_vector[5] = 0
                case dpg.mvKey_L:
                    movement_vector[5] = 0
                case dpg.mvKey_I:
                    movement_vector[4] = 0
                case dpg.mvKey_K:
                    movement_vector[4] = 0
                case dpg.mvKey_Q:
                    movement_vector[3] = 0
                case dpg.mvKey_E:
                    movement_vector[3] = 0
        if pid_enabled:
            client.set_pid_target(movement_vector[0:2], target_depth, target_yaw, target_pitch, target_roll, speed)
        else:
            client.set_manual_thrust(movement_vector, speed)

    with dpg.handler_registry():
        dpg.add_key_press_handler(callback=handle_keyboard_press)
        dpg.add_key_release_handler(callback=handle_keyboard_release)
    dpg.create_viewport()
    dpg.setup_dearpygui()



    # open new window context
    with dpg.window(label="python :3"):
        dpg.add_checkbox(label="pid", callback=pid_en_callback)
        dpg.add_checkbox(label="start sending sensor data (not implemented yet haha)")
        dpg.add_drag_float(label="speed", min_value=0, max_value=30, callback=set_speed)
        dpg.add_drag_float(label="Grip", min_value=1000, max_value=1500, callback=set_grip)
        dpg.add_drag_float(label="Rotation", min_value=1000, max_value=1500, callback=set_rot)
        dpg.add_button(label="reset all pid", callback=reset_all_pid)
        dpg.add_text(label=f"target yaw: {target_yaw}")
        dpg.add_text(label=f"target roll: {target_roll}")
        dpg.add_text(label=f"target pitch: {target_pitch}")
        dpg.add_text(label=f"target depth: {target_depth}")

    with dpg.window(label="yaw pid"):
        dpg.add_text("yaw PID")
        dpg.add_drag_float(label="p const", callback=set_pid_constant, user_data="yaw_p")
        dpg.add_drag_float(label="i const", callback=set_pid_constant, user_data="yaw_i")
        dpg.add_drag_float(label="d const", callback=set_pid_constant, user_data="yaw_d")
        with dpg.plot(label="yaw over time"):
            dpg.add_plot_axis(dpg.mvXAxis, label="time")
            dpg.add_plot_axis(dpg.mvYAxis, label="deg", tag="y_axis")
            dpg.add_hline_series(client.past_data_yaw, parent="y_axis")

    with dpg.window(label="pitch pid"):
        dpg.add_text("pitch PID")
        dpg.add_drag_float(label="p const", callback=set_pid_constant, user_data="pitch_p")
        dpg.add_drag_float(label="i const", callback=set_pid_constant, user_data="pitch_i")
        dpg.add_drag_float(label="d const", callback=set_pid_constant, user_data="pitch_d")
        with dpg.plot(label="pitch over time"):
            dpg.add_plot_axis(dpg.mvXAxis, label="time")
            dpg.add_plot_axis(dpg.mvYAxis, label="deg", tag="y1_axis")
            dpg.add_hline_series(client.past_data_pitch, parent="y1_axis")
    
    with dpg.window(label="roll pid"):
        dpg.add_text("roll PID")
        dpg.add_drag_float(label="p const", callback=set_pid_constant, user_data="roll_p")
        dpg.add_drag_float(label="i const", callback=set_pid_constant, user_data="roll_i")
        dpg.add_drag_float(label="d const", callback=set_pid_constant, user_data="roll_d")
        with dpg.plot(label="roll over time"):
            dpg.add_plot_axis(dpg.mvXAxis, label="time")
            dpg.add_plot_axis(dpg.mvYAxis, label="deg", tag="y2_axis")
            dpg.add_hline_series(client.past_data_roll, parent="y2_axis")
    
    with dpg.window(label="depth pid"):
        dpg.add_text("depth PID")
        dpg.add_drag_float(label="p const", callback=set_pid_constant, user_data="depth_p")
        dpg.add_drag_float(label="i const", callback=set_pid_constant, user_data="depth_i")
        dpg.add_drag_float(label="d const", callback=set_pid_constant, user_data="depth_d")
        with dpg.plot(label="depth over time"):
            dpg.add_plot_axis(dpg.mvXAxis, label="time")
            dpg.add_plot_axis(dpg.mvYAxis, label="meters", tag="y3_axis")
            dpg.add_hline_series(client.past_data_depth, parent="y3_axis")

    dpg.show_viewport()
    dpg.start_dearpygui()
    dpg.destroy_context()

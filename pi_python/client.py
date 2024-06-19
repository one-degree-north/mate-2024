import dearpygui.dearpygui as dpg
import socket, select, queue, threading, struct
from collections import namedtuple
from dataclasses import dataclass
import cv2
import sys
import numpy as np

data_length = 1000
past_data_yaw = [0] * data_length
past_data_pitch = [0] * data_length
past_data_roll = [0] * data_length
past_data_depth = [0] * data_length
x_axis = []
for i in range(data_length):
    x_axis.append(i)


class PIClient:
    #code to communicate to opi
    move = namedtuple("move", ['f', 's', 'u', 'p', 'r', 'y'])
    def __init__(self, server_address=("192.168.2.2", 7774), dpg=None):
        self.server_address = server_address
        self.out_queue = queue.Queue()
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
        # self.past_data_roll = [0] * self.data_length
        # self.past_data_pitch = [0] * self.data_length
        # #self.past_data_yaw = [0] * self.data_length
        # self.past_data_depth = [0] * self.data_length
        # self.x_axis = []
        # for i in range(self.data_length):
        #     self.x_axis.append(i)
        
    def start_client_thread(self):
        self.client_thread = threading.Thread(target=self.client_loop, args=[self.server_address], daemon=True)
        self.client_thread.start()
    
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
        # cmd = struct.unpack("!c", data[0])[0]
        # cmd = struct.unpack("!c", data)
        cmd = data[0]
        if cmd == 0x10: # sensor data!
            sensor_data = struct.unpack("!cBBBBfffff", data)
            print(f"sensor_data: {sensor_data}")
            item_index = 1
            for x, y in self.data.items():
                self.data[x] = sensor_data[item_index]
                item_index += 1
            for i in range(self.data_length-1):
                i1 = self.data_length - i - 1
                past_data_roll[i1] = past_data_roll[i1-1]
                past_data_pitch[i1] = past_data_pitch[i1-1]
                past_data_yaw[i1] = past_data_yaw[i1-1]
                past_data_depth[i1] = past_data_depth[i1-1]
            past_data_roll[0] = self.data["roll"]
            past_data_pitch[0] = self.data["pitch"]
            past_data_yaw[0] = self.data["yaw"]
            past_data_depth[0] = self.data["depth"]
            dpg.set_value("yaw_tag", [x_axis, past_data_yaw])
            dpg.set_value("roll_tag", [x_axis, past_data_roll])
            dpg.set_value("pitch_tag", [x_axis, past_data_pitch])
            dpg.set_value("depth_tag", [x_axis, past_data_depth])
        if cmd == 0x20: # current thruster data!
            pass
        

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
            data_target = bytes([0x01])
            data_type = bytes([0x00])
        if constant == "yaw_i":
            data_target = bytes([0x01])
            data_type = bytes([0x01])
        if constant == "yaw_d":
            data_target = bytes([0x01])
            data_type = bytes([0x02])
        if constant == "depth_p":
            data_target = bytes([0x00])
            data_type = bytes([0x00])
        if constant == "depth_i":
            data_target = bytes([0x00])
            data_type = bytes([0x01])
        if constant == "depth_d":
            data_target = bytes([0x00])
            data_type = bytes([0x02])
        if constant == "pitch_p":
            data_target = bytes([0x02])
            data_type = bytes([0x00])
        if constant == "pitch_i":
            data_target = bytes([0x02])
            data_type = bytes([0x01])
        if constant == "pitch_d":
            data_target = bytes([0x02])
            data_type = bytes([0x02])
        if constant == "roll_p":
            data_target = bytes([0x03])
            data_type = bytes([0x00])
        if constant == "roll_i":
            data_target = bytes([0x03])
            data_type = bytes([0x01])
        if constant == "roll_d":
            data_target = bytes([0x03])
            data_type = bytes([0x01])

        self.out_queue.put(struct.pack("!cccf", bytes([0x040]), data_target, data_type, value))

    def test_connection(self):
        self.out_queue.put(struct.pack("!c", bytes([0x10])))
    
    def reset_all_pid(self):
        self.out_queue.put(struct.pack("!c", bytes([0x41])))
    
    def enable_sensor_feedback(self):
        self.out_queue.put(struct.pack("!c", bytes([0x50])))

    def disable_sensor_feedback(self):
        self.out_queue.put(struct.pack("!c", bytes([0x51])))

    def enable_thruster_feedback(self):
        self.out_queue.put(struct.pack("!c", bytes([0x60])))

    def disable_thruster_feedback(self):
        self.out_queue.put(struct.pack("!c", bytes([0x61])))

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
    min_speed = 0
    max_speed = 30
    claw_rot = 1500
    claw_grip = 1500
    data_length = 1000
    past_data_yaw = [0] * data_length
    key_down = [False]*1000  # I have no idea what the max dpg.mvkey is so whatever
    max_claw_rot = 2500
    min_claw_rot = 500
    max_claw_grip = 2000
    min_claw_grip = 1250
    cam1 = cv2.VideoCapture()
    width1 = cam1.get(cv2.CAP_PROP_FRAME_WIDTH)
    height1 = cam1.get(cv2.CAP_PROP_FRAME_HEIGHT)
    cam1data = None
    cam2 = cv2.VideoCapture()
    width2 = cam2.get(cv2.CAP_PROP_FRAME_WIDTH)
    height2 = cam2.get(cv2.CAP_PROP_FRAME_HEIGHT)
    cam2data = None
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
    client.start_client_thread()

    def handle_keyboard_press(sender, app_data):
        global movement_vector
        global target_roll
        global target_pitch
        global target_yaw
        global target_depth
        global pid_enabled
        global client
        global speed
        global debug
        global key_down
        global claw_rot
        global claw_grip
        changed = False
        if key_down[app_data] == False:
            changed = True
            key_down[app_data] = True
        if changed:
            match(app_data):
                case dpg.mvKey_W:
                    movement_vector[0] = 1
                case dpg.mvKey_S:
                    movement_vector[0] = -1
                case dpg.mvKey_D:
                    movement_vector[1] = 1
                case dpg.mvKey_A:
                    movement_vector[1] = -1
                case dpg.mvKey_C:
                    claw_grip -= 50
                case dpg.mvKey_V:
                    claw_grip += 50
                case dpg.mvKey_U:
                    claw_rot -= 50
                case dpg.mvKey_O:
                    claw_rot += 50
                case dpg.mvKey_Colon:
                    speed -= 1
                case dpg.mvKey_Quote:
                    speed += 1
                case dpg.mvKey_Z:
                    claw_grip = min_claw_grip
                case dpg.mvKey_X:
                    claw_grip = max_claw_grip
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
                        # target_yaw += -5
                        target_yaw = -1
                    case dpg.mvKey_E:
                        # target_yaw += 5
                        target_yaw = 1
                    case dpg.mvKey_Shift:
                        target_depth -= 0.01
                        #target_depth = -1
                    case dpg.mvKey_Spacebar:
                        target_depth += 0.01
                        #target_depth = 1
                target_roll = ((target_roll + 180) % 360) - 180
                target_pitch = ((target_pitch + 180) % 360) - 180
                # target_yaw = ((target_yaw) % 360)
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
                    case dpg.mvKey_Shift:
                        movement_vector[2] = -1
                    case dpg.mvKey_Spacebar:
                        movement_vector[2] = 1
            if claw_grip > max_claw_grip:
                claw_grip = max_claw_grip
            if claw_grip < min_claw_grip:
                claw_grip = min_claw_grip
            if claw_rot > max_claw_rot:
                claw_rot = max_claw_rot
            if claw_rot < min_claw_rot:
                claw_rot = min_claw_rot
            if speed > max_speed:
                speed = max_speed
            if speed < min_speed:
                speed = min_speed
            client.set_grip(claw_grip)
            client.set_rot(claw_rot)
            if pid_enabled:
                client.set_pid_target(movement_vector[0:2], target_depth, target_yaw, target_pitch, target_roll, speed)
            else:
                client.set_manual_thrust(movement_vector, speed)
            dpg.set_value("target_pitch", f"target pitch: {target_pitch}")
            # dpg.set_value("target_yaw", f"target yaw: {target_yaw}")
            dpg.set_value("target_depth", f"target depth: {target_depth}, current depth: {client.data['depth']}")
            dpg.set_value("target_roll", f"target roll: {target_roll}")
            dpg.set_value("claw_grip", f"claw grip: {claw_grip}")
            dpg.set_value("claw_rot", f"claw rot: {claw_rot}")
            dpg.set_value("speed", f"speed: {speed}")

    def handle_keyboard_release(sender, app_data):
        global movement_vector
        global target_roll
        global target_pitch
        global target_yaw
        global target_depth
        global pid_enabled
        global client
        global speed
        global debug
        global key_down
        key_down[app_data] = False
        # if debug:
        #     print(f"key: {app_data}")
        match(app_data):
            case dpg.mvKey_W:
                movement_vector[0] = 0
            case dpg.mvKey_S:
                movement_vector[0] = 0
            case dpg.mvKey_D:
                movement_vector[1] = 0
            case dpg.mvKey_A:
                movement_vector[1] = 0
            # case dpg.mvKey_Shift:
            #     movement_vector[2] = 0
            # case dpg.mvKey_Spacebar:
            #     movement_vector[2] = 0
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
                case dpg.mvKey_Shift:
                    movement_vector[2] = 0
                case dpg.mvKey_Spacebar:
                    movement_vector[2] = 0
        if pid_enabled:
            match(app_data):
                case dpg.mvKey_Q:
                    target_yaw = 0
                case dpg.mvKey_E:
                    target_yaw = 0
                # case dpg.mvKey_Shift:
                #     target_depth = 0
                # case dpg.mvKey_Spacebar:
                #     target_depth = 0

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
        dpg.add_text(f"speed: {speed}", tag="speed")
        dpg.add_text(f"target yaw: {target_yaw}", tag="target_yaw")
        dpg.add_text(f"target roll: {target_roll}", tag="target_roll")
        dpg.add_text(f"target pitch: {target_pitch}", tag="target_pitch")
        dpg.add_text(f"target depth: {target_depth}", tag="target_depth")
        dpg.add_text(f"claw rot: {claw_rot}", tag="claw_rot")
        dpg.add_text(f"claw grip: {claw_grip}", tag="claw_grip")
        dpg.add_checkbox(label="pid", callback=pid_en_callback)
        dpg.add_checkbox(label="start sending sensor data (not implemented yet haha)")
        dpg.add_drag_float(label="speed", min_value=0, max_value=25, callback=set_speed)
        dpg.add_drag_float(label="Grip", min_value=1000, max_value=2000, callback=set_grip)
        dpg.add_drag_float(label="Rotation", min_value=1000, max_value=2000, callback=set_rot)
        dpg.add_button(label="reset all pid", callback=reset_all_pid)

    with dpg.window(label="yaw pid", pos=[750, 0]):
        dpg.add_text("yaw PID")
        dpg.add_drag_float(label="p const", callback=set_pid_constant, user_data="yaw_p")
        dpg.add_drag_float(label="i const", callback=set_pid_constant, user_data="yaw_i")
        dpg.add_drag_float(label="d const", callback=set_pid_constant, user_data="yaw_d")
        with dpg.plot(label="yaw over time"):
            dpg.add_plot_axis(dpg.mvXAxis, label="time")
            dpg.add_plot_axis(dpg.mvYAxis, label="deg", tag="y_axis")
            dpg.add_line_series(x_axis, past_data_yaw,parent="y_axis", tag="yaw_tag")

    with dpg.window(label="pitch pid", pos=[0, 500]):
        dpg.add_text("pitch PID")
        dpg.add_drag_float(label="p const", callback=set_pid_constant, user_data="pitch_p")
        dpg.add_drag_float(label="i const", callback=set_pid_constant, user_data="pitch_i")
        dpg.add_drag_float(label="d const", callback=set_pid_constant, user_data="pitch_d")
        with dpg.plot(label="pitch over time"):
            dpg.add_plot_axis(dpg.mvXAxis, label="time")
            dpg.add_plot_axis(dpg.mvYAxis, label="deg", tag="y1_axis")
            dpg.add_line_series(x_axis, past_data_pitch, parent="y1_axis", tag="pitch_tag")
    
    with dpg.window(label="roll pid", pos = [750, 500]):
        dpg.add_text("roll PID")
        dpg.add_drag_float(label="p const", callback=set_pid_constant, user_data="roll_p")
        dpg.add_drag_float(label="i const", callback=set_pid_constant, user_data="roll_i")
        dpg.add_drag_float(label="d const", callback=set_pid_constant, user_data="roll_d")
        with dpg.plot(label="roll over time"):
            dpg.add_plot_axis(dpg.mvXAxis, label="time")
            dpg.add_plot_axis(dpg.mvYAxis, label="deg", tag="y2_axis")
            dpg.add_line_series(x_axis, past_data_roll, parent="y2_axis", tag="roll_tag")
    
    with dpg.window(label="depth pid", pos = [400, 0]):
        dpg.add_text("depth PID")
        dpg.add_drag_float(label="p const", callback=set_pid_constant, user_data="depth_p")
        dpg.add_drag_float(label="i const", callback=set_pid_constant, user_data="depth_i")
        dpg.add_drag_float(label="d const", callback=set_pid_constant, user_data="depth_d")
        with dpg.plot(label="depth over time"):
            dpg.add_plot_axis(dpg.mvXAxis, label="time")
            dpg.add_plot_axis(dpg.mvYAxis, label="meters", tag="y3_axis")
            dpg.add_line_series(x_axis, past_data_depth, parent="y3_axis", tag="depth_tag")

    with dpg.texture_registry(show=True):
        dpg.add_raw_texture(width=width1, height=height1, default_value=cam1data, tag="cam1", format=dpg.mvFormat_Float_rgb)

    
    while dpg.is_dearpygui_running():
        ret, frame = cam1.read()
        data = np.flip(frame, 2)
        data = data.ravel()
        # dpg.add_raw_texture()
        data = np.asfarray(data, dtype='f')
        texture_data = np.true_divide(data, 255.0)
        dpg.set_value("cam1", texture_data)
        dpg.render_dearpygui_frame()

    dpg.show_viewport()
    dpg.start_dearpygui()
    dpg.destroy_context()

import imgui
import socket, select, queue, threading, struct
from collections import namedtuple
from dataclasses import dataclass

import sys

class PIClient:
    #code to communicate to opi
    move = namedtuple("move", ['f', 's', 'u', 'p', 'r', 'y'])
    def __init__(self, server_address=("192.168.2.2", 7774)):
        self.out_queue = queue.Queue()
        self.client_thread = threading.Thread(target=self.client_loop, args=[server_address], daemon=True)
        self.client_thread.start()
        # self.bno_data = data()
    
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
      
    def move_servo(self, pulse1, pulse2):
        self.out_queue.put(struct.pack("!cHH", bytes([0x20]), int(pulse1//2), int(pulse2//2)))

    def set_manual_thrust(self, moves, speed):
        self.out_queue.put(struct.pack("!cfffffff", bytes([0x20]), *(moves[0:6]), speed))

    def set_pid_target(self, moves, target_depth, target_yaw, target_pitch, target_roll, speed):    # moves are in target m/s
        self.out_queue.put(struct.pack("!cfffffff", bytes([0x21]), *(moves[0:2]), target_depth, target_yaw, target_pitch, target_roll, speed))
 
    def test_connection(self):
        self.out_queue.put(struct.pack("!c", bytes([0x10])))

if __name__ == "__main__":
    client = PIClient()
    pid_enabled = False
    changed = False
    # initilize imgui context (see documentation)
    imgui.create_context()
    imgui.get_io().display_size = 100, 100
    imgui.get_io().fonts.get_tex_data_as_rgba32()

    # start new frame context
    imgui.new_frame()

    # open new window context
    imgui.begin("python :3", True)



    # close current window context
    imgui.end()

    # pass all drawing comands to the rendering pipeline
    # and close frame context
    imgui.render()
    imgui.end_frame()

    movement_vector = [0, 0, 0, 0, 0, 0]
    target_yaw = 0
    target_roll = 0
    target_depth = 0
    target_pitch = 0
    speed = 0
    imgui.core.is_key_pressed
    if (imgui.is_key_pressed(imgui.KEY_W)):
        movement_vector[0] = 1
        changed=True
    if (imgui.is_key_pressed(imgui.KEY_S)):
        movement_vector[0] = -1
        changed=True
    if (imgui.is_key_pressed(imgui.KEY_D)):
        movement_vector[1] = 1
        changed=True
    if (imgui.is_key_pressed(imgui.KEY_A)):
        movement_vector[1] = -1
        changed=True
    if (imgui.is_key_pressed(imgui.KEY_SHIFT)):
        movement_vector[2] = 1
        changed=True
    if (imgui.is_key_pressed(imgui.KEY_SPACE)):
        movement_vector[2] = -1
        changed=True
    if (pid_enabled):
        if (imgui.is_key_pressed(imgui.KEY_J)):
            target_roll -= 5
            changed=True
        if (imgui.is_key_pressed(imgui.KEY_L)):
            target_roll += 5
            changed=True
        if (imgui.is_key_pressed(imgui.KEY_I)):
            target_pitch += 5
            changed=True
        if (imgui.is_key_pressed(imgui.KEY_K)):
            target_pitch += -5
            changed=True
        if (imgui.is_key_pressed(imgui.KEY_Q)):
            target_yaw += -5
            changed=True
        if (imgui.is_key_pressed(imgui.KEY_E)):
            target_yaw += 5
            changed=True
    else:
        if (imgui.is_key_pressed(imgui.KEY_J)):
            movement_vector[5] = -1
            changed=True
        if (imgui.is_key_pressed(imgui.KEY_L)):
            movement_vector[5] = 1
            changed=True
        if (imgui.is_key_pressed(imgui.KEY_I)):
            movement_vector[4] = 1
            changed=True
        if (imgui.is_key_pressed(imgui.KEY_K)):
            movement_vector[4] = -1
            changed=True
        if (imgui.is_key_pressed(imgui.KEY_Q)):
            movement_vector[3] = -1
            changed=True
        if (imgui.is_key_pressed(imgui.KEY_E)):
            movement_vector[3] = 1
            changed=True
    _, pid_enabled = imgui.checkbox("pid", pid_enabled)
    changed_s, speed = imgui.drag_float("speed", min_value=0, max_value=30, value=speed)
    if (changed_s):
        changed = True
    if (changed):
        if (pid_enabled):
            client.set_pid_target(movement_vector[0:2], target_depth, target_yaw, target_depth, target_roll, speed)
        else:
            client.set_manual_thrust(movement_vector, speed)
    changed = False

#server communicates with the surface
import threading, queue, socket, select, struct

class Server:
    def __init__(self, server_address: tuple, stop_event=None, use_stop_event=False, debug=True):
        self.connected = False
        self.server_address = server_address
        self.thruster_control = None
        self.interface = None
        self.client_addr = ()
        self.server_thread = threading.Thread(target=self._server_loop)
        self.out_queue = queue.Queue()

        self.use_stop_event=use_stop_event
        self.debug=debug
        self.stop_event = stop_event
    
    def set_thruster_control(self, thruster_control):
        self.thruster_control = thruster_control

    def set_interface(self, interface):
        self.interface = interface

    # starts the server for surface client to communicate with
    def start_server(self):
        if self.debug:
            print("trying to set up server")
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(self.server_address)
        self.self_addr = self.server_address
        if self.debug:
            print(f"server set up at {self.server_address}")
        self.server_thread.start()
        return self.server_thread

    # read and write data to and from surface client
    def _server_loop(self):
        while True:
            if self.use_stop_event:
                if self.stop_event.is_set():
                    break
            r, w, x = select.select([self.sock], [self.sock], [self.sock])
            for sock in r:  #ready to read!
                if self.debug:
                    print("attempting to read network data")
                data, address = sock.recvfrom(2048)
                if address != self.client_add or not self.connected: #client switched address (something wrong happened)
                    self.client_addr = address
                    self.connected = True
                self._parse_data(data)
            
            for sock in w:  #ready to write!
                if not self.out_queue.empty() and self.connected:
                    if self.debug:
                        print(f"attempting to write to {self.client_addr}")
                    sock.sendto(self.out_queue.get(), self.client_addr)
            
            for sock in x:  #exception 8^(. Create new socket and try to connect again.
                if self.debug:
                    print("exception apparently occured")
    
    #parse data from surface client
    def _parse_data(self, data):
        cmd = data[0]

        if cmd == 0x00: # test connection
            print("test connection")
            # self.interface.test_connection()
        elif cmd == 0x01: # echo
            if len(data) > 0:
                self.interface.echo(data[1:])
        elif cmd == 0x10:   # first connection (just to get addr)
            print("client connected!")
        elif cmd == 0x20:   # set manual thrust
            if len(data) == 29:
                trans = struct.unpack("!fff", data[1:13])
                rot = struct.unpack("!fff", data[13:25])
                speed = struct.unpack("!f", data[25:29])
                print(f"set manual: trans: {trans}, rot: {rot}, speed: {speed}")
                self.interface.set_manual_thrust(trans[0], trans[1], trans[2], rot[0], rot[1], rot[2], speed[0])
        elif cmd == 0x21:   # set pid thrust
            if len(data) == 29:
                trans = struct.unpack("!fff", data[1:13])
                rot = struct.unpack("!fff", data[13:25])
                speed = struct.unpack("!f", data[25:29])
                print(f"set pid: trans: {trans}, depth: {trans[2]}, yaw: {rot[0]} pitch: {rot[1]} roll: {rot[2]}")
                self.interface.set_pid_thrust(trans[0], trans[1], trans[2], rot[0], rot[1], rot[2], speed[0])
        elif cmd == 0x22:   # set manual pos
            pass
        elif cmd == 0x30:   # claw grip
            if len(data) == 5:
                micro = struct.unpack("!I", data[1:5])
                if self.debug:
                    print(f"setting grip: {micro}")
                self.interface.set_grip(micro)
        elif cmd == 0x31:   # claw rot
            if len(data) == 5:
                micro = struct.unpack("!I", data[1:5])
                if self.debug:
                    print(f"setting rot: {micro}")
                self.interface.set_rot(micro)
        elif cmd == 0x40:   # pid change constant
            data_target = struct.unpack("!c", data[1:2]) #0: depth, 1: yaw, 2: pitch, 3: roll
            data_type = struct.unpack("!c", data[2:3])
            value = struct.unpack("!f", data[3:7])
            if data_target == 0x00:
                if data_type == 0x00:
                    self.interface.depth_pid.k_const = value
                if data_type == 0x01:
                    self.interface.depth_pid.i_const = value
                if data_type == 0x02:
                    self.interface.depth_pid.d_const = value
            if data_target == 0x01:
                if data_type == 0x00:
                    self.interface.yaw_pid.k_const = value
                if data_type == 0x01:
                    self.interface.yaw_pid.i_const = value
                if data_type == 0x02:
                    self.interface.yaw_pid.d_const = value
            if data_target == 0x00:
                if data_type == 0x00:
                    self.interface.pitch_pid.k_const = value
                if data_type == 0x01:
                    self.interface.pitch_pid.i_const = value
                if data_type == 0x02:
                    self.interface.pitch_pid.d_const = value
            if data_target == 0x00:
                if data_type == 0x00:
                    self.interface.roll_pid.k_const = value
                if data_type == 0x01:
                    self.interface.roll_pid.i_const = value
                if data_type == 0x02:
                    self.interface.roll_pid.d_const = value
        elif cmd == 0x41:   # reset all pid
            self.interface.depth_pid.reset()
            self.interface.yaw_pid.reset()
            self.interface.pitch_pid.reset()
            self.interface.roll_pid.reset()

            
    def send_sens_data(self, data):
        self.out_queue.put(struct.pack("!cBBBBfffff", bytes([0x10]), *(data.values())))

if __name__ == "__main__":
    pass

# mcu_interface communicates with the microcontroller
from dataclasses import dataclass
import serial, struct, threading, time
# import RPi.GPIO as GPIO
# import pigpio

HEADER = 0xa7
FOOTER = 0x7a

#TODO: fix problem where if footer doesn't match, still thinks it's a valid packet
@dataclass
class Packet:
    cmd: int
    param: int
    len: int
    data: list[int]
    curr_size: int
    complete: bool
    all_bytes: bytes
    def __init__(self):
        self.clear()

    def add_byte(self, byte):
        byte &= 0xFF
        if self.curr_size == 0: # header
            if byte != HEADER:
                return False
            self.header = byte
        elif self.curr_size == 1: # cmd
            self.cmd = byte
        elif self.curr_size == 2: # param
            self.param = byte
        elif self.curr_size == 3: # len
            self.len = byte
        elif self.curr_size > 3 and self.curr_size <= 3+self.len: # data
            self.data.append(byte)
            # self.data[self.curr_size-4] = byte
        elif self.curr_size == 4+self.len:
            if byte == FOOTER: # we're done!
                self.all_bytes += bytes([byte])
                self.complete = True
                return True
            else:
                self.clear()
                return False
        else:   #??????
            print("somehow got out of curr_size")
            self.clear()
            return False
        self.all_bytes += bytes([byte])
        self.curr_size += 1
        return True
    
    def to_bytes(self):
        return self.all_bytes
    
    def to_bytes_network(self):
        return self.all_bytes[1:-1]

    def clear(self):
        self.header=self.cmd=self.param=self.len=self.footer=-1
        self.data = []
        self.complete = False
        self.curr_size = 0
        self.all_bytes = []
    
    def is_complete(self):
        return self.complete
    
    def __repr__(self):
        return f"{self.cmd} {self.param} {self.len} {self.data} {self.curr_size} "

class MCUInterface:
    def __init__(self, serial_port="/dev/ttyACM0", stop_event=None, use_stop_event=False, debug=False, write_delay=0.05, bno_data=None, data_lock=None):
        self.serial_port = serial_port
        # self.ser = serial.Serial(serial_port, 115200, dsrdtr=True, rtscts=True)
        self.init_serial()

        # self.pwm = pigpio.pi()
        # self.pwm.set_mode(12, pigpio.OUTPUT)
        # self.pwm.set_PWM_frequency(12, 50)

        self.ser_enabled = False
        self.read_packet = Packet()
        self.server = None
        self.read_thread = threading.Thread(target=self._read_thread)
        self.write_delay=write_delay

        self.stop_event = stop_event
        self.debug=debug
        self.use_stop_event=use_stop_event

        self.bno_data = bno_data
        self.data_lock = data_lock

        # self.claw_pwm = GPIO.PWM(12, 500)
        # self.claw_pwm.start(0.6)

    # def move_claw(self, open):
    #     if open:
    #         self.pwm.set_servo_pulsewidth(12, 1700)
    #     else:
    #         self.pwm.set_servo_pulsewidth(12, 1200)
        # deg 
        # dc = deg*(0.25/360)+0.75
        # if dc > 1:
        #     dc = 1
        # if dc < 0.2:
        #     dc = 0.2
        # self.claw_pwm.ChangeDutyCycle(dc)

    def init_serial(self):
        if 'ser' in dir(self):
            self.ser.close()
            del self.ser
        self.ser = serial.Serial(self.serial_port, 9600, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, bytesize=serial.EIGHTBITS, timeout=None, write_timeout=0.5)
        self.ser.dtr = True
    
    def set_server(self, server):
        self.server = server

    def start(self):
        self.ser_enabled = True
        if not self.ser.is_open and self.server != None:
            self.ser.open()
        self.read_thread.start()

    # def end_pwm(self):
    #     self.claw_pwm.stop()
        # GPIO.cleanup()

    def _write_packet(self, cmd:int, param:int, data): #WRITE IS BIG ENDIAN!!!!
        try:
            # self.ser.reset_input_buffer()
            out: bytes = struct.pack("<BBBB", HEADER, cmd, param, len(data)) + data + struct.pack("<B", FOOTER)
            print(out.hex(" "))
            self.ser.write(out)
            self.ser.reset_output_buffer()
        except serial.SerialTimeoutException:
            print("err")
            self.ser.reset_output_buffer()
            print("past buffer reset")
            self.init_serial()

    def _read_thread(self): #READ IS LITTLE ENDIAN!!!!!
        while True:
            if self.use_stop_event:
                if self.stop_event.is_set():
                    break
            # I'm not sure if read_all blocks (it should as timeout=None on ser)
            new_bytes = self.ser.read_all()
            for byte in new_bytes:
                if self.debug:
                    # test = struct.pack(">B", byte)
                    print(f"{chr(byte)}", end='')
                self.read_packet.add_byte(byte)
                if self.read_packet.is_complete():
                    self.ser.reset_input_buffer()
                    self._parse(self.read_packet)  # read is LITTLE ENDIAN!!!!
                    self.read_packet.clear()
            time.sleep(self.write_delay)

    def float_to_duty_cycle(self, thrust):  # turns a float (-1 to 1) to the given duty cycle (uint 16)
        # max value = 1/10 of cycle (20 millisecond, 2 millisecond) = 6553 (0x)
        # mid value = 1/15 of cycle = 4369
        # min value = 1/20 of cycle (20 millisecond, 1 millisecond) = 3277
        # if self.debug:
        #     print(f"float to dc thrust: {thrust}")
        clk_thrust = int(((2**16-1)-(2**15))/2*thrust + 2**14*3)
        if clk_thrust > 2**16-1:
            clk_thrust = 2**16-1
        if clk_thrust < 2**15:
            clk_thrust = 2**15
        return int(clk_thrust/10)
    
    def float_to_micro(self, thrust):
        new_thrust = int(1500 + 500*thrust)
        if new_thrust > 1800:
            new_thrust = 1800
        if new_thrust < 1200:
            new_thrust = 1200
        return int(new_thrust)

    # parse data, stores data needed on opi and sends data to surface
    def _parse(self, packet):

        if self.debug:
            print(f"received data: {packet.to_bytes}")
            print(f"received packet cmd: {packet.cmd}, len: {packet.len}, bytes: {packet.to_bytes()}")
            if packet.cmd == 0x1A:
                curr_thrusters = struct.unpack("<HHHHHH", bytes(packet.data))
                print(f"thrusts: {curr_thrusters}")
            if packet.cmd == 0x2A:
                curr_servos = struct.unpack("<HH", bytes(packet.data))
                print(f"servos: {curr_servos}")
            if packet.cmd == 0x10:  # BNO DATA
                if packet.param == 0x00:    #quat
                    data = struct.unpack("<ffff", bytes(packet.data))
                    print(f"quat: {data}")
                elif packet.param == 0x01:  #euler
                    data = struct.unpack("<fff", bytes(packet.data))
                    print(f"euler: {data}")
                elif packet.param == 0x02:  #lin
                    data = struct.unpack("<fff", bytes(packet.data))
                    print(f"lin: {data}")
                elif packet.param == 0x03:  #gyro
                    data = struct.unpack("<fff", bytes(packet.data))
                    print(f"gyro: {data}")
        # store data needed
        if packet.cmd == 0x10:  # BNO DATA
            if packet.param == 0x00:    #quat
                self.data_lock.acquire()
                data = struct.unpack("<ffff", bytes(packet.data))
                self.bno_data["quat"] = data
                self.data_lock.release()
            elif packet.param == 0x01:  #euler
                self.data_lock.acquire()
                data = struct.unpack("<fff", bytes(packet.data))
                self.bno_data["eul"] = data
                self.data_lock.release()
            elif packet.param == 0x02:  #lin
                self.data_lock.acquire()
                data = struct.unpack("<fff", bytes(packet.data))
                self.bno_data["lin"] = data
                self.data_lock.release()
            elif packet.param == 0x03:  #gyro
                self.data_lock.acquire()
                data = struct.unpack("<fff", bytes(packet.data))
                self.bno_data["gyro"] = data
                self.data_lock.release()
        # forward to network
        pkt_len = len(packet.to_bytes())
        if not self.debug:
            self.server.send_data(struct.pack("!" + "B"*(pkt_len-2), *struct.unpack("<" + "B"*(pkt_len-2), bytes(packet.to_bytes_network()))))    # transform little endian into network endianess

    def set_thrusters(self, thrusts):
        u16_thrusts = []
        for i in range(len(thrusts)):
            u16_thrusts.append(self.float_to_duty_cycle(thrusts[i]))
        if self.debug:
            print(f"setting thrusts {thrusts}, with {u16_thrusts}")  
        self._write_packet(0x18, 0x0F, struct.pack("<HHHHHH", *u16_thrusts))
        if self.debug:
            print("finished writing")

    def set_servos(self, vals):
        print(f"setting thrusts {thrusts}")
        self._write_packet(0x28, 0x2F, struct.pack(">HH", *vals))

    def echo(self, vals):
        self._write_packet(0x01, 0x00, vals.encode())

    def test_connection(self):
        self._write_packet(0x00, 0x00, bytes([]))

    def get_thrusters(self):
        self._write_packet(0x1A, 0x0F, bytes([]))

    def get_servos(self):
        self._write_packet(0x2A, 0x2F, bytes([]))

    def funny(self):
        self.ser.write(bytes([0x04]))

class ArduinoInterface:
    def __init__(self, serial_port="/dev/ttyACM0", stop_event=None, use_stop_event=False, debug=False, write_delay=0.05):
        self.serial_port = serial_port
        # self.ser = serial.Serial(serial_port, 115200, dsrdtr=True, rtscts=True)
        self.init_serial()

        # self.pwm = pigpio.pi()
        # self.pwm.set_mode(12, pigpio.OUTPUT)
        # self.pwm.set_PWM_frequency(12, 50)

        self.ser_enabled = False
        self.read_packet = Packet()
        self.server = None
        self.write_delay=write_delay

        self.stop_event = stop_event
        self.debug=debug
        self.use_stop_event=use_stop_event

        # self.bno_data = bno_data
        # self.data_lock = data_lock

        # self.claw_pwm = GPIO.PWM(12, 500)
        # self.claw_pwm.start(0.6)

    # def move_claw(self, open):
    #     if open:
    #         self.pwm.set_servo_pulsewidth(12, 1700)
    #     else:
    #         self.pwm.set_servo_pulsewidth(12, 1200)
        # deg 
        # dc = deg*(0.25/360)+0.75
        # if dc > 1:
        #     dc = 1
        # if dc < 0.2:
        #     dc = 0.2
        # self.claw_pwm.ChangeDutyCycle(dc)

    def init_serial(self):
        if 'ser' in dir(self):
            self.ser.close()
            del self.ser
        self.ser = serial.Serial(self.serial_port, 115200, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, bytesize=serial.EIGHTBITS, timeout=None, write_timeout=0.5)
        # self.ser = serial.Serial(self.serial_port, 115200)
        # self.ser.dtr = True

    def start(self):
        self.ser_enabled = True
        if not self.ser.is_open:
            self.ser.open()

    # def end_pwm(self):
    #     self.claw_pwm.stop()
        # GPIO.cleanup()



    def _write_packet(self, cmd:int, data): #WRITE IS BIG ENDIAN!!!!
        try:
            # self.ser.reset_input_buffer()
            out: bytes = struct.pack("<c", bytes([cmd])) + data
            print(out.hex(" "))
            self.ser.write(out)
            self.ser.reset_output_buffer()
        except serial.SerialTimeoutException:
            print("err")
            self.ser.reset_output_buffer()
            print("past buffer reset")
            self.init_serial()
   
    def float_to_micro(self, thrust):
        micros = int(1500 + 500*thrust)
        if micros > 1800:
            micros = 1800
        if micros < 1200:
            micros = 1200
        return micros

    def set_thrusters(self, thrusts):
        u16_thrusts = []
        for i in range(len(thrusts)):
            u16_thrusts.append(self.float_to_micro(thrusts[i]))
        #     u16_thrusts.append(self.float_to_duty_cycle(thrusts[i]))
        if self.debug:
            print(f"setting thrusts {thrusts}, with {u16_thrusts}")  
        self._write_packet(0x69, struct.pack("<HHHHHH", *u16_thrusts))
        if self.debug:
            print("finished writing")

    def set_rotate_servo(self, val):
        print(f"setting rotate servo {val}")
        self._write_packet(0x50, struct.pack("<H", val))
    
    def set_grip_servo(self, val):
        print(f"setting grip servo {val}")
        self._write_packet(0x40, struct.pack("<H", val))

if __name__ == "__main__":
    interface = ArduinoInterface("/dev/ttyACM0", debug=True)
    while True:
        val = input("input type > ")
        if val == "all":
            t = float(input("thrust: "))
            thrusts = [t, t, t, t, t, t]
            interface.set_thrusters(thrusts)
        if val == "st":
            t = float(input("microseconds: "))
            thruster = int(input("thruster: "))
            thrusts = [0]*6
            thrusts[thruster] = t
            interface.set_thrusters(thrusts)
        if val == "grip":
            t = float(input("microseconds: "))
            interface.set_grip_servo(t)
        if val == "rot":
            t = float(input("microseconds: "))
            interface.set_rotate_servo(t)
    # data_lock=threading.Lock()
    # interface = MCUInterface("/dev/ttyACM0", debug=True, bno_data=bno_data,data_lock=data_lock)
    # interface.start()
    # while True:
    #     val = input("input type > ")
    #     if val == "a":
    #         interface.funny()
    #     if val == "all":
    #         t = float(input("thrust: "))
    #         thrusts = [t, t, t, t, t, t]
    #         interface.set_thrusters(thrusts)
    #     if val == "st":
    #         thruster = int(input("thruster: "))
    #         microseconds = float(input("microseconds: "))
    #         thrusts = [0, 0, 0, 0, 0, 0]
    #         thrusts[thruster] = microseconds
    #         interface.set_thrusters(thrusts)
    #     if val == "sv":
    #         servo1 = int(input("servo 1: "))
    #         servo2 = int(input("servo 2: "))
    #         vals = [servo1, servo2]
    #         interface.set_servos(vals)
    #     if val == "bruh":
    #         interface.test_connection()
    #     if val == "echo":
    #         interface.echo(input("> "))
    #     if val == "thrust":
    #         interface.get_thrusters()
    #     if val == "servos":
    #         interface.get_servos()
    #     if val == 'b':
    #         u16_thrusts = [51256, 49152, 49152, 49152, 49152, 49152]
    #         interface._write_packet(0x18, 0x0F, struct.pack(">HHHHHH", *u16_thrusts))
    #     # if val == "claw":
    #     #     oc = input("claw o/c")
    #     #     interface.move_claw(True if oc == 'o' else False)
    #     # if val == "dclaw":
    #     #     interface.end_pwm()

    
            

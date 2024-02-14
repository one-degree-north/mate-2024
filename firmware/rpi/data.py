# interperates and stores IMU data. Also autoreports data to surface.
from dataclasses import dataclass
from queue import Queue
import threading
import time

import sys
from pathlib import Path
path = Path(sys.path[0])
sys.path.insert(1, str((path.parent.parent).absolute()))

import logging
import time

from Adafruit_BNO055 import BNO055

# bno = BNO055.BNO055(serial_port='/dev/serial0', rst=18)

# bno = BNO055.BNO055()

class DataInput:
    def __init__(self, report_data=False, stop_event=None, use_stop_event=False, debug=False):
        self.bno = BNO055.BNO055()
        self.bno_read_delay = 0.01    # in seconds
        self.data_thread = threading.Thread(target=self.bno_loop)

        self.data = {
            "acceleration":[0,0,0],
            "linear_acceleration":[0,0,0],
            "gyroscope":[0,0,0],
            "euler":[0,0,0],
            "quaterion":[0,0,0,0],
            "temperature":[0,0,0],
            "magnetometer":[0,0,0],
            "calibration":[0,0,0],
            "gravity":[0,0,0]
        }

        self.debug=debug
        if self.debug:
            self.last_time = time.process_time_ns()
    
    def start_bno_reading(self):
        self.data_thread.start()

    def bno_loop(self):
        while True:
             # Read the Euler angles for heading, roll, pitch (all in degrees).
            self.data["euler"] = self.bno.read_euler()
            # Read the calibration status, 0=uncalibrated and 3=fully calibrated.
            self.data["calibration"] = self.bno.get_calibration_status()
            # Print everything out.
            # print('Heading={0:0.2F} Roll={1:0.2F} Pitch={2:0.2F}\tSys_cal={3} Gyro_cal={4} Accel_cal={5} Mag_cal={6}'.format(
            #     heading, roll, pitch, sys, gyro, accel, mag))
            # Other values you can optionally read:
            # Orientation as a quaternion:
            self.data["quaterion"] = self.bno.read_quaterion()
            # Sensor temperature in degrees Celsius:
            self.data["temperature"] = self.bno.read_temp()
            # Magnetometer data (in micro-Teslas):
            self.data["magnetometer"] = self.bno.read_magnetometer()
            # Gyroscope data (in degrees per second):
            self.data["gyroscope"] = self.bno.read_gyroscope()
            # Accelerometer data (in meters per second squared):
            self.data["acceleration"] = self.bno.read_accelerometer()
            # Linear acceleration data (i.e. acceleration from movement, not gravity--
            # returned in meters per second squared):
            self.data["linear_acceleration"] = self.bno.read_linear_acceleration()
            # Gravity acceleration data (i.e. acceleration just from gravity--returned
            # in meters per second squared):
            self.data["gravity"] = self.bno.read_gravity()
            # Sleep for a second until the next reading.

            if self.debug:
                now = time.process_time_ns
                print(f"delta time: {self.last_time-now}")
                print(f"data: {self.data}")
                self.last_time = now

            time.sleep(self.bno_read_delay)


if __name__ == "__main__":
    data_input = DataInput()
    data_input.start_bno_reading()
    while True:
        time.sleep(1)
        print(data_input.data)

# # Enable verbose debug logging if -v is passed as a parameter.
# if len(sys.argv) == 2 and sys.argv[1].lower() == '-v':
#     logging.basicConfig(level=logging.DEBUG)

# # Initialize the BNO055 and stop if something went wrong.
# if not bno.begin():
#     raise RuntimeError('Failed to initialize BNO055! Is the sensor connected?')

# # Print system status and self test result.
# status, self_test, error = bno.get_system_status()
# print('System status: {0}'.format(status))
# print('Self test result (0x0F is normal): 0x{0:02X}'.format(self_test))
# # Print out an error if system status is in error mode.
# if status == 0x01:
#     print('System error: {0}'.format(error))
#     print('See datasheet section 4.3.59 for the meaning.')

# # Print BNO055 software revision and other diagnostic data.
# sw, bl, accel, mag, gyro = bno.get_revision()
# print('Software version:   {0}'.format(sw))
# print('Bootloader version: {0}'.format(bl))
# print('Accelerometer ID:   0x{0:02X}'.format(accel))
# print('Magnetometer ID:    0x{0:02X}'.format(mag))
# print('Gyroscope ID:       0x{0:02X}\n'.format(gyro))

# print('Reading BNO055 data, press Ctrl-C to quit...')
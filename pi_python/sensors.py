from adafruit_extended_bus import ExtendedI2C as I2C
import board
import adafruit_bno055
# from adafruit_BNO055 import BNO055
import ms5837
import tsys01
import time

class Sensors():
    def __init__(self):
        self.read_delay = 0.01
        self.i2c = I2C(1)
        self.bno = adafruit_bno055.BNO055_I2C(self.i2c)
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
        self.past_data = []
        self.depth_sensor = ms5837.MS5837(model=ms5837.MODEL_30BA, bus=1)
        self.depth_sensor.init()
        self.depth_sensor.setFluidDensity(997) # Use predefined saltwater density
        self.bno.offsets_accelerometer = []
        self.bno.offsets_gyroscope = []
        self.bno.offsets_magnetometer = []
        self.bno.mode = adafruit_bno055.NDOF_MODE
        self.temp_sensor = tsys01.TSYS01() # Use default I2C bus 1
        self.temp_sensor.init()
    def read_sensors(self):
        self.depth_sensor.read()
        self.data["depth"] = self.depth_sensor.depth()
        self.data["yaw"], self.data["roll"], self.data["pitch"] = self.bno.euler
        self.data["sys_calib"], self.data["gyro_calib"], self.data["accel_calib"], self.data["mag_calib"] = self.bno.calibration_status
        self.temp_sensor.read()
        self.data["temp"] = self.temp_sensor.temperature()

if __name__ == "__main__":
    sensor = Sensors()
    while True:
        time.sleep(0.01)
        sensor.read_sensors()
        print(sensor.data)
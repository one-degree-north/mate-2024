from adafruit_extended_bus import ExtendedI2C as I2C
import board
import adafruit_bno055
# from adafruit_BNO055 import BNO055
import ms5837
import tsys01
import time

class Sensors():
    def __init__(self, debug=True):
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
        #self.bno.mode = adafruit_bno055.CONFIG_MODE
        #self.bno.offsets_accelerometer = (-29, -36, -34)
        #self.bno.offsets_gyroscope = (-1, 1, 0)
        #self.bno.offsets_magnetometer = (-157, -182, 93)
        self.bno.mode = adafruit_bno055.NDOF_MODE
        self.temp_sensor = tsys01.TSYS01() # Use default I2C bus 1
        self.temp_sensor.init()
        self.debug = debug
    def read_sensors(self):
        curr_bno_eul = self.bno.euler
        self.depth_sensor.read()
        self.temp_sensor.read()
        if (curr_bno_eul[0] == None or curr_bno_eul[1] == None or curr_bno_eul[2]==None or self.depth_sensor.depth()==None or self.temp_sensor.temperature()==None):
            return 0
        if (curr_bno_eul[0] > 360 or curr_bno_eul[0] < 0 or curr_bno_eul[1] > 180 or curr_bno_eul[1] < -180 or curr_bno_eul[2] > 180 or curr_bno_eul[2] < -180):
            return 0
        self.data["depth"] = self.depth_sensor.depth() * -1
        self.data["yaw"], self.data["roll"], self.data["pitch"] = curr_bno_eul
        # flip roll and pitch because bno orientation
        temp = self.data["pitch"]
        self.data["pitch"] = self.data["roll"]
        self.data["roll"] = temp
        self.data["sys_calib"], self.data["gyro_calib"], self.data["accel_calib"], self.data["mag_calib"] = self.bno.calibration_status
        self.data["temp"] = self.temp_sensor.temperature()

if __name__ == "__main__":
    sensor = Sensors()
    while True:
        time.sleep(0.01)
        sensor.read_sensors()
        print(sensor.data)

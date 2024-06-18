from sensors import Sensors
from server import Server
from controls import Controls

if __name__ == "__main__":
    sensors = Sensors()
    controls = Controls(sensors=sensors)
    server = Server()
    server.set_interface(controls)
    controls.start_loop()
    server._server_loop()
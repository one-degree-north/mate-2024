from mcu_interface import MCUInterface
from server import OPiServer
from thruster_controller import ThrusterController
from data import DataInput
import threading
# import yappi
import time
import json


def main(stop_event, use_stop_event=False, server_debug=False, data_debug=False, thrust_controller_debug=False, mcu_debug=False):
    # setup configuration file contents
    config_file = open('config.json', 'r')
    config = json.loads(config_file.read())
    config_file.close()

    # create components
    addr = config['ip_addr']
    serial_port = config['serial_port']
    thrust_controller = ThrusterController(move_delta_time=config['thruster_move_delta'], stop_event=stop_event, use_stop_event=use_stop_event, debug=not not config['thruster_controller_debug'], passthrough=True)
    server = OPiServer(server_address=(addr, config['udp_port']), stop_event=stop_event, use_stop_event=use_stop_event, debug=config["server_debug"])
    interface = MCUInterface(serial_port=serial_port, stop_event=stop_event, use_stop_event=use_stop_event, debug=config["mcu_debug"])

    # resolve dependencies between components
    thrust_controller.set_interface(interface)
    server.set_thruster_control(thrust_controller)
    server.set_interface(interface)
    interface.set_server(server)

    data_input =None
    # config if we are using bno data
    if config['use_bno']:
        opi_data = DataInput(report_data=True, stop_event=stop_event, use_stop_event=use_stop_event, debug=config["data_debug"])
        thrust_controller.set_data(DataInput)
        data_input.set_server(server)

    # start the system
    print("starting server")
    server.start_server()
    print("starting interface")
    interface.start()
    if config['use_bno']:
        print("starting BNO")
        opi_data.start_bno_reading()
    print("starting thruster controller")
    thrust_controller.start_loop()
    
    return thrust_controller, server, opi_data, interface

if __name__ == "__main__":
    # thrust_controller, server, data, interface = main(debug=True)
    main()
    # yappi.start()
    # stop_event = threading.Event()
    # thrust_controller, server, opi_data, interface = main(stop_event, True, False)
    # time.sleep(1)
    # stop_event.set()
    # server.server_thread.join()
    # yappi.stop()

    # # retrieve thread stats by their thread id (given by yappi)
    # threads = yappi.get_thread_stats()
    # for thread in threads:
    #     print(
    #         "Function stats for (%s) (%d)" % (thread.name, thread.id)
    #     )  # it is the Thread.__class__.__name__
    #     yappi.get_func_stats(ctx_id=thread.id).print_all()
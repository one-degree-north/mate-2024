import socket
import time

server_ip = "10.0.0.49"  # Update this to the IP address of your ESP8266
server_port = 80  # The port your ESP8266 TCP server is listening on

def receive_data():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((server_ip, server_port))
        print("Connected to ESP8266. Listening for data packets...")
        
        try:
            while True:
                # Assuming the ESP8266 sends data periodically, we listen for it
                data = s.recv(1024)  # Buffer size of 1024 bytes
                if data:
                    print(f"Received: {data.decode()}")
                else:
                    # If no data is received, we can optionally send a command or just continue listening
                    pass
                time.sleep(1)  # Adjust based on how frequently data is sent
        except KeyboardInterrupt:
            print("Program terminated by user.")
        
if __name__ == "__main__":
    receive_data()

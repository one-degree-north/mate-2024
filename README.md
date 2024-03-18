## MATE 2024 Blue Lobster
This is the GitHub repo for the 2024 Blue Lobster MATE robotics team.

## Setup
### Laptop (Windows)
1. Install and setup [DHCP server](https://www.dhcpserver.de/) for port `192.168.1.2` (Ethernet)

### Raspberry Pi
1. SSH into the Pi over ethernet (using DHCP server)
2. Clone [MJPG-Streamer](https://github.com/jacksonliam/mjpg-streamer) into your user folder
3. Run `chmod +x run.sh` in `mjpg-streamer/mjpg-streamer-experimental`

## Usage
### Laptop (Windows)
* Run `python3 main.py` to start up the GUI

### Raspberry Pi
* Run `sudo python3 main.py` in `firmware/rpi` for driving
* Run `./run.sh` in `mjpg-streamer/mjpg-streamer-experimental` inside a screen session for camera
* Run `sudo python3 mcu_interface.py` in `firmware/rpi` for debugging thrusters and claw

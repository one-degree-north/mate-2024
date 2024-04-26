# Blue Lobster

## What is Mate? 🛳️
The **Blue Lobster**, successor to the infamous Red Lobster, 
is the SAS MATE team for the 2024 MATE ROV competition.
In short, it entails building an underwater ROV, and
remotely operating said ROV to complete a series of tasks.
These tasks require the ROV to be maneuverable, reliable,
and able to perform a variety of functions including but not 
limited to
- picking up and dropping objects
- interacting with objects underwater
- analysing collected data
- reading sensor data 
- using data for decision-making and feedback control
- stream a live video feed to the operator
- use said video feed to both navigate and complete tasks
- use photogrammetry to recreate underwater environments
- and more!

For a more detailed view of required tasks, read [this document](https://20693798.fs1.hubspotusercontent-na1.net/hubfs/20693798/2024%20RANGER%20Manual%20FINAL_1_16_2024_with_Cover.pdf).

## Design Philosophy ✨
Rapid prototyping enables a team to develop a product iteratively. 
It empowers members to take the initiative, but designing an ROV 
for rapid prototyping requires essential decisions on both team 
organization and ROV design to implement this approach effectively. 
During rapid prototyping, issues arise due to unexpected interactions 
between prototypes and existing work and misunderstandings between 
teams. We addressed this issue by modularizing the ROV into systems 
and delegating them to individual subteams. Furthermore, we modularized 
individual core systems, allowing for rapid replacement of standalone 
electromechanical and software components. To facilitate system 
integration, we organized our team around weekly meetings that 
encouraged mutual understanding of how other systems worked. 
This approach allowed us to test several experimental solutions 
before integration, resulting in an innovative and reliable ROV.

## Setup

### Raspberry PI
 - Should have pigpiod and gstreamer installed
 - Should have a static IP address (192.168.1.2 used normally)
 - Should have a camera connected (for camera stream)
 - Should have a BNO055 IMU connected (for rotational controller)
 - Should have a MS5837-30BA connected (for depth controller)

Setup
1. Edit `/system/systemd/system/pigpiod.service` to remove the `-l` option from the pigpiod startup to allow for the client computer to connect to it.
2. Then Run the `sudo systemctl enable pigpiod --now` to enable it on boot.
3. Add a file called `run` at `/opt/pigpio/cgi` with the following contents:
```bash
#!/bin/bash
"$@"
```
4. Make it executable with `sudo chmod +x /opt/pigpio/cgi/run`
5. Install gstreamer with `sudo apt install gstreamer1.0-tools gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly`

### Client (OSX)
- Should be connected to the Raspberry PI on the same network

Setup
1. Install gstreamer with `brew install gstreamer`
2. Install cmake and ninja with `brew install cmake ninja`
3. Clone this repository with `git clone https://github.com/one-degree-north/mate-2024-blue-lobster -b sids-cpp-port`
4. Run `cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release .`
5. Run `ninja -C build`
6. Run `./build/client` to start the client



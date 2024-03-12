screen -S camera -dm bash -c 'mjpg-streamer/mjpg-streamer-experimental/run.sh'
echo camera started on screen session: 'camera'
cd firmware/rpi
sudo python3 main.py
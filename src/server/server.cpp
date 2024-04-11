extern "C" {
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
}

#include <gst/gst.h>
#include <gst/app/app.h>

#include <chrono>
#include <thread>
#include <mutex>
#include <algorithm>
#include <termios.h>

#include <sys/ioctl.h>
#include <fcntl.h>

#include "comms.h"

std::string get_client_address() {
    if (const char* client_address_env = getenv("CLIENT_ADDRESS"))
        return client_address_env;

    std::string client_address;
    std::cout << "Enter Client Address: ";
    std::cin >> client_address;

    return client_address;
}

std::mutex bno_mutex;
std::string client_address = get_client_address();
Communication communication(client_address, 7070);
bool isConfiguringBNO = true;
int bno;

int i2c_smbus_write_byte_data(int file, uint8_t reg, uint8_t value);
int i2c_smbus_read_byte_data(int file, uint8_t reg);

void bno_data_thread() {
    bno = open("/dev/i2c-1", O_RDWR);
    if (bno < 0) {
        std::cerr << "Error: Unable to open I2C device" << std::endl;
        std::exit(1);
    }

    int bno_address = 0x28;
    if (ioctl(bno, I2C_SLAVE, bno_address) < 0) {
        std::cerr << "Error: Unable to set I2C device address" << std::endl;
        std::exit(1);
    }

    i2c_smbus_write_byte_data(bno, 0x3D, 0); // Go to config mode
    uint8_t chip_id = i2c_smbus_read_byte_data(bno, 0x00);
    if (chip_id != 0xA0) {
        std::cerr << "Error: Invalid chip ID" << std::endl;
        std::exit(1);
    }

    i2c_smbus_write_byte_data(bno, 0x3F, 0x20); // Reset
    std::this_thread::sleep_for(std::chrono::milliseconds(650)); // Wait for sensor to reset

    i2c_smbus_write_byte_data(bno, 0x3E, 0); // Normal Power Mode
    i2c_smbus_write_byte_data(bno, 0x3F, 0); // Use Internal Oscillator

    i2c_smbus_write_byte_data(bno, 0x3D, 0x0C); // Go to NDOF mode
    i2c_smbus_write_byte_data(bno, 0x3B, 0); // Unit Selection
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Wait for sensor to initialize

    while (true) {
        bno_mutex.lock();
        if (isConfiguringBNO) {
            uint8_t calib_stat = i2c_smbus_read_byte_data(bno, 0x35);

            communication.send({0x03, calib_stat});
        } else {
            std::vector<uint8_t> data(46);
            data[0] = 0x04;

            for (int i = 1; i <= 46; i++)
                data[i] = i2c_smbus_read_byte_data(bno, 0x08 + (i - 1));
            communication.send(data);
        }
        bno_mutex.unlock();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int open_feather() {
    int serial_port = open("/dev/ttyACM0", O_RDWR | O_NONBLOCK | O_NOCTTY);
    if (serial_port < 0) {
        std::cerr << "Error: Unable to open serial port" << std::endl;
        std::exit(1);
    }
    std::cout << "Opened serial port" << std::endl;

    struct termios tty;
    if (tcgetattr(serial_port, &tty) != 0) {
        std::cerr << "Error: Unable to get serial port attributes" << std::endl;
        std::exit(1);
    }

    tty.c_cflag &= ~PARENB; // No parity
    tty.c_cflag &= ~CSTOPB; // One stop bit
    tty.c_cflag &= ~CSIZE; // Clear data size
    tty.c_cflag |= CS8; // 8 bits per byte
    tty.c_cflag &= ~CRTSCTS; // No hardware flow control
    tty.c_cflag |= CREAD | CLOCAL; // Enable reading and ignore control lines

    tty.c_lflag &= ~ICANON; // Disable canonical mode
    tty.c_lflag &= ~ECHO; // Disable echo
    tty.c_lflag &= ~ECHOE; // Disable erasure
    tty.c_lflag &= ~ECHONL; // Disable newline echo
    tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

    tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

    tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
    tty.c_cc[VMIN] = 0;

    cfsetspeed(&tty, B115200); // Set baud rate

    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
        return 1;
    }
    std::cout << "Set serial port attributes" << std::endl;

    return serial_port;
}

int main() {
//    std::thread bno_thread(bno_data_thread);

    int serial_port = open_feather();

    gst_init(NULL, NULL);

    std::string gst_pipeline = "v4l2src device=/dev/video0 ! rtpjpegpay ! udpsink port=6970 host=" + client_address;
//    int video_stream = open("/dev/video0", O_RDONLY);
//    if (video_stream < 0) {
//        std::cerr << "Error: Unable to open video stream. Using test stream." << std::endl;
//        gst_pipeline = "videotestsrc ! jpegenc " + gst_pipeline;
//    } else {
//        gst_pipeline = "v4l2src device=/dev/video0" + gst_pipeline;
//        close(video_stream);
//    }

    std::cout << "Using pipeline: " << gst_pipeline << std::endl;
    GError *error = nullptr;
    GstElement *pipeline = gst_parse_launch(gst_pipeline.c_str(), &error);
    if (error) {
        std::cerr << "Error: Unable to create pipeline. " << error->message << std::endl;
        std::exit(1);
    }

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    std::vector<uint8_t> buffer;
    while (true) {
        if (communication.recv(buffer)) {
            const size_t n = buffer.size();
            std::cout << buffer[0] << std::endl;
            switch (buffer[0]) {
                case 0x01: { // ping
                    if (n != 1) goto invalid;

                    std::cout << "Pinged" << std::endl;
                    communication.send({0x02});
                    break;
                }
                case 0x02: { // set thruster info
                    std::cout << "Received " << n << "bytes" << std::endl;
                    if (n != 25) goto invalid;

                    #define DOUBLE_TO_THRUSTER_MS(x) std::clamp((uint16_t) (1000 + 500 * (x + 1)), (uint16_t) 1000, (uint16_t) 2000)

                    union {
                        struct {float forward, side, up, pitch, yaw, roll;};
                        uint8_t buffer[25];
                    } thruster_info{};
                    std::copy(buffer.begin() + 1, buffer.begin() + 25, thruster_info.buffer);
                    const int thruster_pins[] = {5, 1, 2, 4, 0, 3};

                    union {
                        #pragma pack(push, 1)
                        struct {
                            uint8_t cmd_byte;
                            unsigned short total_thrust[6];
                        } data;
                        #pragma pack(pop)

                        uint8_t buffer[13];
                    } thruster_command {.data = {.cmd_byte = 0x69}};

                    thruster_command.data.total_thrust[thruster_pins[0]] = DOUBLE_TO_THRUSTER_MS((thruster_info.forward - thruster_info.side - thruster_info.yaw) / 30.0);
                    thruster_command.data.total_thrust[thruster_pins[1]] = DOUBLE_TO_THRUSTER_MS((thruster_info.forward + thruster_info.side + thruster_info.yaw) / 30.0);
                    thruster_command.data.total_thrust[thruster_pins[2]] = DOUBLE_TO_THRUSTER_MS((thruster_info.forward - thruster_info.side + thruster_info.yaw) / 30.0);
                    thruster_command.data.total_thrust[thruster_pins[3]] = DOUBLE_TO_THRUSTER_MS((thruster_info.forward + thruster_info.side - thruster_info.yaw) / 30.0);
                    thruster_command.data.total_thrust[thruster_pins[4]] = DOUBLE_TO_THRUSTER_MS((thruster_info.up - thruster_info.roll) / 20.0);
                    thruster_command.data.total_thrust[thruster_pins[5]] = DOUBLE_TO_THRUSTER_MS((thruster_info.up + thruster_info.roll) / 20.0);

                    printf("[%0.2f, %0.2f, %0.2f, %0.2f, %0.2f, %0.2f]\n", thruster_info.forward, thruster_info.side, thruster_info.up, thruster_info.pitch, thruster_info.roll, thruster_info.yaw);

                    std::cout << "[";
                    for (int i = 0; i < 6; i++)
                        std::cout << thruster_command.data.total_thrust[i] << ", ";
                    std::cout << "]" << std::endl;

//                    thruster_command.data.total_thrust[thruster_pins[0]] = 1600;
//                    thruster_command.data.total_thrust[thruster_pins[1]] = 1600;
//                    thruster_command.data.total_thrust[thruster_pins[2]] = 1600;
//                    thruster_command.data.total_thrust[thruster_pins[3]] = 1600;
//                    thruster_command.data.total_thrust[thruster_pins[4]] = 1600;
//                    thruster_command.data.total_thrust[thruster_pins[5]] = 1600;

//                    for (int i = 1; i < 13; i += 2)
//                        std::swap(thruster_command.buffer[i], thruster_command.buffer[i + 1]);

                    for (int i = 0; i < 13; i++)
                        printf("%x ", thruster_command.buffer[i]);
                    std::cout << std::endl;

                    ssize_t s = write(serial_port, thruster_command.buffer, 13);
                    if (s < 0) {
                        std::cerr << "Error: Unable to write to serial port" << std::endl;
                    } else {
                        std::cout << "Wrote " << s << " bytes to serial port" << std::endl;
                    }

                    break;
                }
                case 0x03: { // set BNO055 calibration status
                    if (n != 2) goto invalid;

                    std::cout << "Set BNO055 calibration status to " << (buffer[1] == 0 ? "Configuring" : "Sensor") << std::endl;

                    bno_mutex.lock();
                    isConfiguringBNO = buffer[1] == 0;
                    bno_mutex.unlock();

                    break;
                }
                case 0x04: {
                    if (n != 5) goto invalid;

                    float value = *((float*) &buffer[1]);
                    uint16_t claw_cycle = std::clamp((uint16_t) (500 + 1000 * (value + 1)), (uint16_t) 500, (uint16_t) 2500);

                    const uint8_t cmd[] = {0x50, (uint8_t) (claw_cycle & 0xFF), (uint8_t) (claw_cycle >> 8)};
                    if (write(serial_port, cmd, 3) < 0) std::cout << "Failed to write to claw" << std::endl;

                    break;
                }
                case 0x05: {
                    if (n != 5) goto invalid;

                    float value = *((float*) &buffer[1]);
                    uint16_t claw_cycle = std::clamp((uint16_t) (500 + 1000 * (value + 1)), (uint16_t) 500, (uint16_t) 2500);

                    const uint8_t cmd[] = {0x40, (uint8_t) (claw_cycle & 0xFF), (uint8_t) (claw_cycle >> 8)};
                    for (int i = 0; i < 3; i++)
                        std::cout << (int) cmd[i] << " ";
                    if (write(serial_port, cmd, 3) < 0) std::cout << "Failed to write to claw" << std::endl;

                    break;
                }
                default:
                invalid:
                    std::cerr << "Error: Invalid message" << std::endl;
                    continue;
            }
        }
    }
}
extern "C" {
    #include <linux/i2c-dev.h>
    #include <i2c/smbus.h>
}

#include <gst/gst.h>
#include <gst/app/app.h>

#include <chrono>
#include <thread>
#include <mutex>

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

int main() {
    std::thread bno_thread(bno_data_thread);

    int video_stream = open("/dev/video0", O_RDONLY);
    if (video_stream < 0) {
        std::cerr << "Error: Unable to open video stream" << std::endl;
        std::exit(1);
    }
    close(video_stream);

    GstElement *pipeline = gst_parse_launch((R"(v4l2src device=/dev/video0 ! "image/jpeg, width=1920, height=1080" ! rtpjpegpay ! udpsink port=6970 host=")" + client_address + "\"").c_str()  , NULL);
    if (!pipeline) {
        std::cerr << "Error: Unable to create pipeline" << std::endl;
        std::exit(1);
    }

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    std::vector<uint8_t> buffer;
    while (true) {
        if (communication.recv(buffer)) {
            const size_t n = buffer.size();
            switch (buffer[0]) {
                case 0x01: { // ping
                    if (n != 1) goto invalid;

                    std::cout << "Pinged" << std::endl;
                    communication.send({0x02});
                    break;
                }
                case 0x02: { // set thruster info
                    if (n != 13) goto invalid; // 6 * 2 (sizeof(short int)) + 1

                    std::cout << "Set thruster info" << std::endl;
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
                default:
                invalid:
                    std::cerr << "Error: Invalid message" << std::endl;
                    continue;
            }
        }
    }
}
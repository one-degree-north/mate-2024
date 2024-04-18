//
// Created by Sidharth Maheshwari on 17/4/24.
//
#include "pigpiod_if2.h"

#include "pi.h"

Pi::Pi(std::string server_address) {
    this->pi_handle = pigpio_start(server_address.c_str(), "8888");
}

Pi::~Pi() {
    pigpio_stop(this->pi_handle);
}

int Pi::Shell(std::string scriptName, std::string arguments) {
    return shell_(this->pi_handle, (char*) scriptName.c_str(), (char*) arguments.c_str());
}

int Pi::SetServoPulseWidth(int gpio, int pulse_width) {
    if (gpio == 12 || gpio == 13) { // Hardware PWM pins
        hardware_PWM(this->pi_handle, gpio, 50, pulse_width / 20000.0 * 1000000);
    } else {
        set_servo_pulsewidth(this->pi_handle, gpio, pulse_width);
    }
}

int Pi::OpenI2C(int bus, int address) {
    return i2c_open(this->pi_handle, bus, address, 0);
}

int Pi::CloseI2C(int handle) {
    return i2c_close(this->pi_handle, handle);
}

int Pi::WriteI2CByte(int handle, int byte) {
    return i2c_write_byte(this->pi_handle, handle, byte);
}

int Pi::ReadI2CByte(int handle) {
    return i2c_read_byte(this->pi_handle, handle);
}

int Pi::WriteI2CByteData(int handle, int reg, int byte) {
    return i2c_write_byte_data(this->pi_handle, handle, reg, byte);
}

int Pi::ReadI2CByteData(int handle, int reg) {
    return i2c_read_byte_data(this->pi_handle, handle, reg);
}
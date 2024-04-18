//
// Created by Sidharth Maheshwari on 17/4/24.
//
#include "pigpiod_if2.h"

#include <iostream>

#include "pi.h"

Pi::Pi(const std::string& server_address) {
    if ((this->pi_handle = pigpio_start(server_address.c_str(), "8888")) < 0) {
        std::cerr << "Failed to connect to pigpiod" << std::endl;
        exit(1);
    }
}

Pi::~Pi() {
    pigpio_stop(this->pi_handle);
}

int Pi::SetGPIOMode(int gpio, int mode) const {
    return set_mode(this->pi_handle, gpio, mode);
}

int Pi::Shell(const std::string& scriptName, const std::string& arguments) const {
    return shell_(this->pi_handle, (char*) scriptName.c_str(), (char*) arguments.c_str());
}

int Pi::SetServoPulseWidth(int gpio, int pulse_width) const {
    if (gpio == 12 || gpio == 13) { // Hardware PWM pins
        return hardware_PWM(this->pi_handle, gpio, 50, pulse_width / 20000.0 * 1000000);
    } else {
        return set_servo_pulsewidth(this->pi_handle, gpio, pulse_width);
    }
}

int Pi::OpenI2C(int bus, int address) const {
    return i2c_open(this->pi_handle, bus, address, 0);
}

int Pi::CloseI2C(int handle) const {
    return i2c_close(this->pi_handle, handle);
}

int Pi::WriteI2CByte(int handle, int byte) const {
    return i2c_write_byte(this->pi_handle, handle, byte);
}

int Pi::ReadI2CByte(int handle) const {
    return i2c_read_byte(this->pi_handle, handle);
}

int Pi::WriteI2CByteData(int handle, int reg, int byte) const {
    return i2c_write_byte_data(this->pi_handle, handle, reg, byte);
}

int Pi::ReadI2CByteData(int handle, int reg) const {
    return i2c_read_byte_data(this->pi_handle, handle, reg);
}

int Pi::WriteI2CWordData(int handle, int reg, int word) const {
    return i2c_write_word_data(this->pi_handle, handle, reg, word);
}

int Pi::ReadI2CWordData(int handle, int reg) const {
    return i2c_read_word_data(this->pi_handle, handle, reg);
}

int Pi::WriteI2CDevice(int handle, char *data, int length) const {
    return i2c_write_device(this->pi_handle, handle, data, length);
}

int Pi::ReadI2CDevice(int handle, char* data, int length) const {
    return i2c_read_device(this->pi_handle, handle, data, length);
}
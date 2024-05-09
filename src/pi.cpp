#include "pigpiod_if2.h"

#include <iostream>

#include "pi.h"

Pi::Pi(const std::string& server_address) {
    this->pi_handle_ = pigpio_start(server_address.c_str(), "8888");
    if (pi_handle_ < 0)
        throw std::runtime_error("Failed to connect to pigpiod");
}

Pi::~Pi() {
    pigpio_stop(this->pi_handle_);
}

int Pi::SetGPIOMode(int gpio, int mode) const {
    return set_mode(this->pi_handle_, gpio, mode);
}

int Pi::Shell(const std::string& scriptName, const std::string& arguments) const {
    return shell_(this->pi_handle_, (char*) scriptName.c_str(), (char*) arguments.c_str());
}

int Pi::SetServoPulseWidth(int gpio, int pulse_width) const {
    if (gpio == 12 || gpio == 13) { // Hardware PWM pins
        return hardware_PWM(this->pi_handle_, gpio, 50, pulse_width / 20000.0 * 1000000);
    } else {
        return set_servo_pulsewidth(this->pi_handle_, gpio, pulse_width);
    }
}

int Pi::OpenI2C(int bus, uint8_t address) const {
    return i2c_open(this->pi_handle_, bus, address, 0);
}

int Pi::CloseI2C(int handle) const {
    return i2c_close(this->pi_handle_, handle);
}

int Pi::WriteI2CByte(int handle, uint8_t byte) const {
    return i2c_write_byte(this->pi_handle_, handle, byte);
}

int Pi::ReadI2CByte(int handle) const {
    return i2c_read_byte(this->pi_handle_, handle);
}

int Pi::WriteI2CByteData(int handle, uint8_t reg, uint8_t byte) const {
    return i2c_write_byte_data(this->pi_handle_, handle, reg, byte);
}

int Pi::ReadI2CByteData(int handle, uint8_t reg) const {
    return i2c_read_byte_data(this->pi_handle_, handle, reg);
}

int Pi::WriteI2CWordData(int handle, uint8_t reg, uint16_t word) const {
    return i2c_write_word_data(this->pi_handle_, handle, reg, word);
}

int Pi::ReadI2CWordData(int handle, uint8_t reg) const {
    return i2c_read_word_data(this->pi_handle_, handle, reg);
}

int Pi::ZipI2C(int handle, char* data, int length) const {
    return i2c_zip(this->pi_handle_, handle, data, length, nullptr, 0);
}

int Pi::WriteI2CBlockData(int handle, uint8_t reg, char* data, int length) const {
    return i2c_write_i2c_block_data(this->pi_handle_, handle, reg, data, length);
}

int Pi::ReadI2CBlockData(int handle, uint8_t reg, char* data, int length) const {
    return i2c_read_i2c_block_data(this->pi_handle_, handle, reg, data, length);
}
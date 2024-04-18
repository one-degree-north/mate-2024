//
// Created by Sidharth Maheshwari on 17/4/24.
//

#include <string>

#ifndef MATE_PI_H
#define MATE_PI_H


class Pi {
private:
    int pi_handle;
public:
    explicit Pi(const std::string& server_address);
    ~Pi();

    // GPIO Methods
    int SetGPIOMode(int gpio, int mode) const;

    // Shell Methods
    int Shell(const std::string& scriptName, const std::string& arguments) const;

    // PWM Methods
    int SetServoPulseWidth(int gpio, int pulse_width) const;

    // I2C Methods
    int OpenI2C(int bus, int address) const;
    int CloseI2C(int handle) const;

    int WriteI2CByte(int handle, int byte) const;
    int ReadI2CByte(int handle) const;

    int WriteI2CByteData(int handle, int reg, int byte) const;
    int ReadI2CByteData(int handle, int reg) const;

    int WriteI2CWordData(int handle, int reg, int word) const;
    int ReadI2CWordData(int handle, int reg) const;

    int WriteI2CDevice(int handle, char* data, int length) const;
    int ReadI2CDevice(int handle, char* data, int length) const;
};


#endif // MATE_PI_H

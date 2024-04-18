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
    explicit Pi(std::string server_address);
    ~Pi();

    // Shell Methods
    int Shell(std::string scriptName, std::string arguments);

    // PWM Methods
    int SetServoPulseWidth(int gpio, int pulse_width);

    // I2C Methods
    int OpenI2C(int bus, int address);
    int CloseI2C(int handle);

    int WriteI2CByte(int handle, int byte);
    int ReadI2CByte(int handle);

    int WriteI2CByteData(int handle, int reg, int byte);
    int ReadI2CByteData(int handle, int reg);
};


#endif // MATE_PI_H

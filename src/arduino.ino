#include <Servo.h>

const int PINS[] = {5, 6, 9, 10, 11, 4};
Servo THRUSTERS[6] {};

const int CLAWPINS[] = {12, 13};
Servo CLAWS[2] {};

void setup() {
    Serial.begin(115200);

    for (int i = 0; i < 6; i++) THRUSTERS[i].attach(PINS[i]);
    for (int i = 0; i < 2; i++) CLAWS[i].attach(CLAWPINS[i]);
}

void loop() {
    if (!Serial.available()) return;

    switch (Serial.read()) {
        case 0x69: {
            uint16_t thrusters[6] {};
            Serial.readBytes((char*) thrusters, 12);
            for (int i = 0; i < 6; i++) THRUSTERS[i].writeMicroseconds(thrusters[i]);
            break;
        }
        case 0x50: {
            uint16_t value;
            Serial.readBytes((char*) &value, 2);

            CLAWS[0].writeMicroseconds(value);
            break;
        }
        case 0x40: {
            uint16_t value;
            Serial.readBytes((char*) &value, 2);

            CLAWS[1].writeMicroseconds(value);
        }
    }
}


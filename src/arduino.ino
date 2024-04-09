#include <Servo.h>

const int PINS[] = {5, 6, 9, 10, 11, 4};
Servo THRUSTERS[6] {};

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < 6; i++) THRUSTERS[i].attach(PINS[i]);
}

void loop() {
  if (!Serial.available()) return;

  union {
    u_int16_t thrusters[6];
    char buff[12];
  } cmd;
  Serial.readBytes(cmd.buff, 12);
  
  for (int i = 0; i < 6; i++)
      THRUSTERS[i].writeMicroseconds(cmd.thrusters[i]);
}


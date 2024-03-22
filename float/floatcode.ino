#include <ESP8266WiFi.h>
#include <Wire.h>
#include "MS5837.h"
#include <Stepper.h>

MS5837 sensor;
const int stepsPerRevolution = 700;  // adju later

const int dirPin = D5;  
const int stepPin = D4;  

Stepper myStepper(stepsPerRevolution, dirPin, stepPin);

const char* ssid = "iphone";     
const char* password = "88888888";
float pressure = 0.0;
float depth = 0.0;
const float pressureThreshold = 1307.55;  // estimated
bool depthControlEnabled = true;
bool movingDown = false;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting");

  if (!sensor.init()) {
    Serial.println("Sensor init failed!");
    while(1);  
  }
  sensor.setModel(MS5837::MS5837_30BA);
  sensor.setFluidDensity(997); // pool density?

  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);

  // connect to hotspot

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (Serial.available() > 0) {
    String receivedData = Serial.readStringUntil('\n');
    receivedData.trim();

    if (receivedData == "down") { 
      movingDown = true;
    }
  }

  if (movingDown) {
    if (!depthControlEnabled || (depthControlEnabled && pressure <= pressureThreshold)) {

      digitalWrite(dirPin, LOW);  
      for(int x = 0; x < stepsPerRevolution; x++) {
        digitalWrite(stepPin, HIGH);
        delayMicroseconds(1000);
        digitalWrite(stepPin, LOW);
        delayMicroseconds(1000);
      }
    } else {
      movingDown = false;
    }
  }

  updateSensors();
  logData();
}

void updateSensors() {
  sensor.read(); 
  pressure = sensor.pressure();  
  depth = pressureToDepth(pressure); 
}

void logData() {
  String dataPacket = String(companyNumber) + "," + String(millis()) + "," + String(pressure) + "," + String(depth);
  Serial.println(dataPacket);
}

float pressureToDepth(float pressure) {
  return (pressure - 1013.25) / 10.0;
}

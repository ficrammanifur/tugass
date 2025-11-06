#include <Wire.h>

void setup() {
  Serial.begin(115200);
  Wire.begin(8,9); // ganti sesuai pin SDA/SCL kamu
  Serial.println("Scanning I2C devices...");
  
  byte count = 0;
  for (byte i = 1; i < 127; i++) {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0) {
      Serial.print("I2C device found at 0x");
      Serial.println(i, HEX);
      count++;
    }
  }
  if (count == 0) Serial.println("No I2C devices found!");
}

void loop() {}

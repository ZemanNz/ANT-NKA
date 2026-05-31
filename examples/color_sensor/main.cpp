#include <Wire.h>
#include <Adafruit_TCS34725.h>
#include "robotka.h"

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);
float r, g, b;

void setup() {
    Serial.begin(115200);
    rkConfig cfg; 
    rkSetup(cfg);
    delay(50);
    pinMode(14, PULLUP);
    pinMode(26, PULLUP);
  
    // 1) Spust obě I2C sběrnice
    Wire.begin(14, 26, 400000);
    Wire.setTimeOut(1);
    // 2) Inicializuj senzory:
    // Initialize the color sensor with a unique name and the I2C bus
    rkColorSensorInit("front", Wire, tcs);
}

void loop() {
  // Retrieve RGB values from the sensor named "front"
  if (rkColorSensorGetRGB("front", &r, &g, &b)) {
    Serial.print("R: "); Serial.print(r, 3);
    Serial.print(" G: "); Serial.print(g, 3);
    Serial.print(" B: "); Serial.println(b, 3);
  } else {
    Serial.println("Sensor 'front' not found.");
  }

  delay(1000); // Wait for a second before the next reading
}

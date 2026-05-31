#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include <string.h>
#include "robotka.h"

#define MAX_LASERS  6

struct LaserSensor {
  const char*         name;
  Adafruit_VL53L0X*   lox;
  TwoWire*            bus;
  uint8_t             xshut_pin;
  uint8_t             address;
  bool                initialized; 
};
static LaserSensor laserSensors[MAX_LASERS];
static uint8_t      laserCount = 0;

/**
 * @brief Inicializuje jeden VL53L0X na zadané sběrnici a XSHUT pinu, přepíše mu  adresu.
 */
void rk_laser_init_basic(const char*       name,
                         TwoWire&          bus,
                         Adafruit_VL53L0X& lox,
                         uint8_t           xshut_pin,
                         uint8_t           new_address)
{
  if (laserCount >= MAX_LASERS) {
    Serial.println("ERROR: Max pocet laseru prekrocen");
    return;
  }
  LaserSensor& s = laserSensors[laserCount++];
  s.name      = name;
  s.lox       = &lox;
  s.bus       = &bus;
  s.xshut_pin = xshut_pin;
  s.address   = new_address;
  s.initialized = false;

  // 2) Vyjmi z resetu
  digitalWrite(xshut_pin, HIGH);
  delay(100);

  if (lox.begin(new_address, /*debug=*/false, &bus)) {
    Serial.println(" ");
  } else {
    // Pokud selže, vrať se k výchozímu procesu
    if (!lox.begin(0x29, /*debug=*/false, &bus)) {
      Serial.print("  FAILED boot default "); Serial.println(name);
      return;
    }
    bus.beginTransmission(0x29);
    bus.write(0x8A);
    bus.write(new_address);
    bus.endTransmission();
    if (!lox.begin(new_address, /*debug=*/false, &bus)) {
      Serial.print("  FAILED boot new addr "); Serial.println(name);
      return;
    }
  }  // 4) Přepiš adresu v registrech
  s.initialized = true;
}

/**
 * @brief Přečte jednorázově vzdálenost z jednoho senzoru podle jména.
 */
int rk_laser_measure_basic(const char* name) {
  for (uint8_t i = 0; i < laserCount; i++) {
    LaserSensor& s = laserSensors[i];
    if (strcmp(s.name, name) == 0) {
      if (!s.initialized) {
        return -1;
      }
      VL53L0X_RangingMeasurementData_t m;
      s.lox->rangingTest(&m, /*debug=*/false);
      if (m.RangeStatus != 4) {
        return m.RangeMilliMeter;
      } else {
        return -1;
      }
    }
  }
  return -1;
}

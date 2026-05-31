#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>

// --- UART Configuration ---
#define UART_TX_PIN 17 // Using GPIO26 for Serial2 TX, as 16/17 are used by sensors
#define UART_RX_PIN 16 // Using GPIO25 for Serial2 RX

// --- Sensor Configuration ---
#define SENSOR_COUNT 4
const uint8_t xshutPins[SENSOR_COUNT] = {15, 4, 5, 18}; 

// --- Data Structures ---
// Structure for sending sensor data
typedef struct __attribute__((packed)) {
    uint8_t sensor_id;
    uint16_t distance; // mm
} SensorData;

// --- Global variables ---
Adafruit_VL53L0X sensors[SENSOR_COUNT];

// --- Communication Functions ---
// Function to send data with sync bytes over Serial2
void uartSend(const void* data, size_t size) {
    const uint8_t SYNC0 = 0xAA;
    const uint8_t SYNC1 = 0x55;
    
    Serial2.write(SYNC0);
    Serial2.write(SYNC1);
    Serial2.write((const uint8_t*)data, size);
}

// --- Setup ---
void setup() {
  // Initialize primary serial for logging
  Serial.begin(115200);
  delay(100);
  Serial.println("ESP32 VL53L0X Sender Started!");

  // Initialize secondary serial for data communication
  Serial2.begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
  Serial.println("Serial2 communication initialized for sensor data.");

  // Initialize I2C bus
  Wire.begin(); 

  // --- Sensor Initialization ---
  // Put all sensors in reset
  for (int i = 0; i < SENSOR_COUNT; i++) {
    pinMode(xshutPins[i], OUTPUT);
    digitalWrite(xshutPins[i], LOW);
  }
  delay(20);

  Serial.println("VL53L0X sensor initialization...");

  // Initialize each sensor one by one
  for (int i = 0; i < SENSOR_COUNT; i++) {
    digitalWrite(xshutPins[i], HIGH);
    delay(10); 

    if (!sensors[i].begin(0x29, false, &Wire)) {
      Serial.print("Failed to boot VL53L0X sensor ");
      Serial.println(i);
    } else {
      uint8_t new_addr = 0x30 + i;
      sensors[i].setAddress(new_addr);
      Serial.print("Sensor ");
      Serial.print(i);
      Serial.print(" initialized, address changed to 0x");
      Serial.println(new_addr, HEX);
    }
  }

  Serial.println("\nAll sensors initialized. Starting measurements.");
}

// --- Main Loop ---
void loop() {
  SensorData data_to_send;
  VL53L0X_RangingMeasurementData_t measure;
  
  for (int i = 0; i < SENSOR_COUNT; i++) {
    sensors[i].rangingTest(&measure, false);
    
    data_to_send.sensor_id = (uint8_t)i;

    if (measure.RangeStatus != 4) { // 4 indicates phase out of range
      data_to_send.distance = measure.RangeMilliMeter;
    } else {
      data_to_send.distance = 65535; // UINT16_MAX indicates an error
    }

    // Send the data structure over Serial2 using the sync protocol
    uartSend(&data_to_send, sizeof(data_to_send));

    // Print to local serial for debugging
    Serial.printf("Sent via UART2: Sensor %d, Distance: %d mm\n", 
                  data_to_send.sensor_id, data_to_send.distance);
  }
  
  delay(50); // Wait for a second before sending the next batch
}
#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <cmath>
#include "robotka.h"
#include "_librk_context.h"
#include "Movement.h" // pro definici MoveResult

struct GyroState {
    Adafruit_MPU6050 mpu;
    float angleZ = 0.0f;
    float offsetZ = 0.0f;
    float rawOffsetZ = 0.0f;
    bool initialized = false;
    volatile bool calibrateRequested = false;
    
    // Diagnostické proměnné
    float lastRawGyroZ = 0.0f;
    float lastRawAccZ = 0.0f;
    bool lastReadOk = false;
};

inline GyroState& getGyroState() {
    static GyroState state;
    return state;
}

inline void gyroTask(void *pvParameters) {
    auto &state = getGyroState();
    unsigned long lastTime = micros();
    int task_loop_count = 0;
    
    while (true) {
        task_loop_count++;
        if (state.calibrateRequested) {
            state.calibrateRequested = false;
            float sumZ = 0.0f;
            const int samples = 100;
            int successful_reads = 0;
            for (int i = 0; i < samples; i++) {
                sensors_event_t a_c, g_c, temp_c;
                if (state.mpu.getEvent(&a_c, &g_c, &temp_c)) {
                    successful_reads++;
                    sumZ += g_c.gyro.z;
                }
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            state.rawOffsetZ = sumZ / (successful_reads > 0 ? successful_reads : 1);
            state.angleZ = 0.0f;
            state.offsetZ = 0.0f;
            lastTime = micros();
            Serial.printf("[gyroTask] Recalibrated. Successful reads: %d/%d, new rawOffsetZ: %.6f rad/s\n", 
                          successful_reads, samples, state.rawOffsetZ);
            continue;
        }

        unsigned long currentTime = micros();
        float dt = (float)(currentTime - lastTime) / 1000000.0f;
        lastTime = currentTime;

        sensors_event_t a, g, temp;
        bool ok = state.mpu.getEvent(&a, &g, &temp);
        state.lastReadOk = ok;

        float gz = 0.0f;
        if (ok) {
            state.lastRawGyroZ = g.gyro.z;
            state.lastRawAccZ = a.acceleration.z;
            
            gz = g.gyro.z - state.rawOffsetZ;
            // Apply a small noise deadband
            if (std::abs(gz) < 0.015f) gz = 0.0f;

            // Integrate angular velocity (converted to degrees)
            state.angleZ += gz * (180.0f / M_PI) * dt;
        } else {
            if (task_loop_count % 1000 == 0) {
                Serial.println("[gyroTask] ERROR: Sensor read failed!");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5)); // 200 Hz update rate
    }
}

inline void gyroInit() {
    auto &state = getGyroState();
    if (state.initialized) {
        Serial.println("[gyroInit] Gyro is already initialized.");
        return;
    }

    Serial.println("[gyroInit] Initializing I2C Wire on pins 14 (SDA), 26 (SCL)...");

    // Inicializace I2C sběrnice na pinech SDA=14 a SCL=26
    Wire.begin(14, 26, 100000);
    delay(100);

    Serial.println("[gyroInit] Scanning I2C bus...");
    int nDevices = 0;
    for (byte address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        byte error = Wire.endTransmission();
        if (error == 0) {
            Serial.printf("[gyroInit] I2C device found at address 0x%02X\n", address);
            nDevices++;
        } else if (error == 4) {
            Serial.printf("[gyroInit] Unknown error at address 0x%02X\n", address);
        }
    }
    if (nDevices == 0) {
        Serial.println("[gyroInit] No I2C devices found!");
    } else {
        Serial.printf("[gyroInit] I2C scan completed. Found %d device(s).\n", nDevices);
    }

    bool status = false;
    uint8_t detected_addr = 0;
    
    Serial.println("[gyroInit] Checking MPU6050 address 0x68...");
    if (state.mpu.begin(0x68, &Wire)) {
        status = true;
        detected_addr = 0x68;
    } else {
        Serial.println("[gyroInit] MPU6050 not found at 0x68. Checking address 0x69...");
        if (state.mpu.begin(0x69, &Wire)) {
            status = true;
            detected_addr = 0x69;
        }
    }

    if (!status) {
        Serial.println("=================================================================");
        Serial.println("CRITICAL ERROR: Failed to find MPU-6050 chip at 0x68 or 0x69!");
        Serial.println("Please check I2C connection, power supply and SDA/SCL pull-ups.");
        Serial.println("=================================================================");
        return;
    }

    Serial.printf("[gyroInit] MPU-6050 chip found successfully at address: 0x%02X\n", detected_addr);

    // Nastavení parametrů senzoru
    state.mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    state.mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    state.mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    delay(100);

    Serial.println("=============================================");
    Serial.println("Calibrating MPU-6050... KEEP ROBOT STILL!");
    Serial.println("=============================================");

    float sumZ = 0.0f;
    const int samples = 200;
    int successful_reads = 0;
    
    for (int i = 0; i < samples; i++) {
        sensors_event_t a, g, temp;
        bool ok = state.mpu.getEvent(&a, &g, &temp);
        if (ok) {
            successful_reads++;
            sumZ += g.gyro.z;
            if (i < 5 || i == 100) {
                Serial.printf("Sample %d: raw_gyro_z = %.6f rad/s (%.3f deg/s), acc_z = %.3f m/s^2\n", 
                              i, g.gyro.z, g.gyro.z * 180.0f / M_PI, a.acceleration.z);
            }
        } else {
            Serial.printf("Sample %d: READ FAILED!\n", i);
        }
        delay(10);
    }
    state.rawOffsetZ = sumZ / (successful_reads > 0 ? successful_reads : 1);
    state.angleZ = 0.0f;
    state.offsetZ = 0.0f;
    state.initialized = true;

    Serial.printf("MPU-6050 Calibration completed.\nSuccessful reads: %d/%d, Calculated rawOffsetZ: %.6f rad/s\n", 
                  successful_reads, samples, state.rawOffsetZ);
    Serial.println("=============================================");

    // Vytvoření FreeRTOS tasku na Core 1
    BaseType_t task_created = xTaskCreatePinnedToCore(
        gyroTask,
        "gyroTask",
        4096,
        NULL,
        1,
        NULL,
        1
    );
    Serial.printf("[gyroInit] xTaskCreatePinnedToCore returned: %d\n", task_created);
}

inline float gyroGetAngleZ() {
    auto &state = getGyroState();
    return state.angleZ - state.offsetZ;
}

inline void gyroResetZ() {
    auto &state = getGyroState();
    state.offsetZ = state.angleZ;
}

inline void gyroRequestCalibration() {
    auto &state = getGyroState();
    state.calibrateRequested = true;
}

/**
 * \brief Jízda rovně stabilizovaná gyroskopem.
 */
inline MoveResult move_straight_gyro(float mm, float speed, uint32_t timeout_ms = 5000) {
    if (mm == 0 || speed == 0) { return {true, 0.0f}; }

    bool reverse = (mm < 0);
    float encoder_to_real = 1965.0f / 1973.0f;
    float real_target_mm = std::abs(mm);

    // Vynulování odometrie
    rkMotorsSetPositionLeft(0);
    rkMotorsSetPositionRight(0);
    delay(50);

    gyroResetZ();

    float min_speed = 18.0f;
    float abs_target_speed = std::abs(speed);
    if (abs_target_speed < min_speed) { abs_target_speed = min_speed; }

    float current_base_speed = 0.0f;
    float speed_sign = reverse ? -1.0f : 1.0f;

    float accel_step = abs_target_speed / 50.0f;
    float decel_step = abs_target_speed / 30.0f;

    float kp_gyro = 1.5f; // Citlivost gyro regulace

    unsigned long start_time = millis();

    auto clamp_speed = [](float s) -> int8_t {
        if (s > 100.0f) return 100;
        if (s < -100.0f) return -100;
        return static_cast<int8_t>(s);
    };

    float avg_pos = 0.0f;
    int loop_counter = 0;

    while (true) {
        loop_counter++;
        rb::Manager::get().motor(rk::gCtx.motors().idRight()).requestInfo(nullptr);
        float pos_l = std::abs(rkMotorsGetPositionLeft(true));
        float pos_r = std::abs(rkMotorsGetPositionRight(false));
        avg_pos = (pos_l + pos_r) / 2.0f;
        float real_pos = avg_pos * encoder_to_real;

        if (real_pos >= real_target_mm) {
            break;
        }

        if (millis() - start_time > timeout_ms) {
            rkMotorsSetSpeed(0, 0);
            return {false, reverse ? -real_pos : real_pos};
        }

        float dist_remaining = real_target_mm - real_pos;
        float abs_curr = std::abs(current_base_speed);

        // Brzdná dráha a rampy
        float required_decel_distance = 4.0f * abs_curr;
        if (dist_remaining <= required_decel_distance) {
            if (loop_counter % 2 == 1) {
                abs_curr -= decel_step;
            }
            if (abs_curr < min_speed) abs_curr = min_speed;
        } else {
            if (abs_curr < abs_target_speed) {
                if (loop_counter % 2 == 1) {
                    abs_curr += accel_step;
                }
                if (abs_curr > abs_target_speed) abs_curr = abs_target_speed;
            }
        }
        current_base_speed = abs_curr * speed_sign;

        // Gyro korekce směru
        float heading = gyroGetAngleZ();
        float correction = heading * kp_gyro;
        if (reverse) {
            correction = -correction;
        }

        float speed_l_abs = abs_curr - correction;
        float speed_r_abs = abs_curr + correction;

        rkMotorsSetSpeed(clamp_speed(speed_l_abs * speed_sign), clamp_speed(speed_r_abs * speed_sign));
        delay(10);
    }

    rkMotorsSetSpeed(0, 0);
    float real_traveled = avg_pos * encoder_to_real;
    return {true, reverse ? -real_traveled : real_traveled};
}

/**
 * \brief Otočení o zvolený úhel pomocí gyroskopu.
 * \param target_angle Úhel otočení ve stupních (kladný doleva, záporný doprava).
 */
inline void turn_gyro(float target_angle, float speed, uint32_t timeout_ms = 5000) {
    if (target_angle == 0 || speed <= 0) return;

    rkMotorsSetSpeed(0, 0);
    delay(200);

    gyroResetZ();
    delay(50);

    float max_speed = std::abs(speed);
    float min_speed = 15.0f;
    float ramp_up_deg = 15.0f;
    float ramp_down_deg = 30.0f;

    bool turn_left = (target_angle > 0);
    float abs_target_angle = std::abs(target_angle);

    unsigned long start_time = millis();

    auto clamp_speed = [](float s) -> int8_t {
        if (s > 100.0f) return 100;
        if (s < -100.0f) return -100;
        return static_cast<int8_t>(s);
    };

    while (true) {
        if (millis() - start_time > timeout_ms) break;

        float current_angle = gyroGetAngleZ();
        float abs_current = std::abs(current_angle);
        float error = abs_target_angle - abs_current;

        if (error <= 0.0f) break;

        // Rozjezdová rampa
        float speed_accel = max_speed;
        if (abs_current < ramp_up_deg) {
            speed_accel = min_speed + (max_speed - min_speed) * (abs_current / ramp_up_deg);
        }

        // Brzdná rampa
        float speed_decel = max_speed;
        if (error < ramp_down_deg) {
            speed_decel = min_speed + (max_speed - min_speed) * (error / ramp_down_deg);
        }

        float curr_speed = std::min(speed_accel, speed_decel);

        if (turn_left) {
            rkMotorsSetSpeed(clamp_speed(-curr_speed), clamp_speed(curr_speed));
        } else {
            rkMotorsSetSpeed(clamp_speed(curr_speed), clamp_speed(-curr_speed));
        }
        delay(10);
    }
    rkMotorsSetSpeed(0, 0);
}

/**
 * \brief Dorovnání se na cílový globální úhel gyroskopu.
 */
inline void align_gyro(float target_global_angle, float speed, uint32_t timeout_ms = 3000) {
    unsigned long start_time = millis();
    float kp = 1.2f;
    float min_speed = 12.0f;
    float max_speed = std::abs(speed);

    auto clamp_speed = [](float s) -> int8_t {
        if (s > 100.0f) return 100;
        if (s < -100.0f) return -100;
        return static_cast<int8_t>(s);
    };

    while (millis() - start_time < timeout_ms) {
        float current_angle = gyroGetAngleZ();
        float error = target_global_angle - current_angle;

        if (std::abs(error) < 0.5f) {
            break;
        }

        float target_speed = error * kp;
        if (std::abs(target_speed) < min_speed) {
            target_speed = (error > 0) ? min_speed : -min_speed;
        }
        if (std::abs(target_speed) > max_speed) {
            target_speed = (error > 0) ? max_speed : -max_speed;
        }

        rkMotorsSetSpeed(clamp_speed(-target_speed), clamp_speed(target_speed));
        delay(10);
    }
    rkMotorsSetSpeed(0, 0);
}

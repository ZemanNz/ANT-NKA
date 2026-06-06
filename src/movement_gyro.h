#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <cmath>
#include "robotka.h"
#include "_librk_context.h"
#include "Movement.h" // pro definici MoveResult

// Kalibrační koeficient pro MPU-6050.
// Hodnota 1.28f kompenzuje jak hardwarovou odchylku čipu, tak setrvačnost (doběh) podvozku.
const float GYRO_SCALE_CORRECTION = 1.28f;

struct GyroState {
    Adafruit_MPU6050 mpu;
    float angleZ = 0.0f;
    float offsetZ = 0.0f;
    float rawOffsetZ = 0.0f;
    bool initialized = false;
    volatile bool calibrateRequested = false;
    volatile bool isCalibrating = false;
    
    // Diagnostické proměnné
    float lastRawGyroZ = 0.0f;
    float lastRawAccZ = 0.0f;
    float lastFilteredGyroZ = 0.0f;
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
            state.isCalibrating = true;
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
            state.lastFilteredGyroZ = 0.0f;
            lastTime = micros();
            state.isCalibrating = false;
            Serial.printf("[gyroTask] Recalibrated. Successful reads: %d/%d, new rawOffsetZ: %.6f rad/s\n", 
                          successful_reads, samples, state.rawOffsetZ);
            continue;
        }

        unsigned long currentTime = micros();
        float dt = (float)(currentTime - lastTime) / 1000000.0f;
        if (dt > 0.05f) dt = 0.05f; // Omezení dt při neočekávané systémové prodlevě
        lastTime = currentTime;

        sensors_event_t a, g, temp;
        bool ok = state.mpu.getEvent(&a, &g, &temp);
        state.lastReadOk = ok;

        float gz = 0.0f;
        if (ok) {
            state.lastRawGyroZ = g.gyro.z;
            state.lastRawAccZ = a.acceleration.z;
            
            float raw_gz = g.gyro.z - state.rawOffsetZ;
            
            // Široký filtr pro odhození pouze extrémního I2C šumu (>30.0 rad/s = 1700 deg/s).
            // To zajišťuje, že žádné reálné vibrace ani otáčení nebudou oříznuty.
            if (std::abs(raw_gz) < 30.0f) {
                gz = raw_gz;
                state.lastFilteredGyroZ = raw_gz;
            } else {
                gz = state.lastFilteredGyroZ; // Podržíme předchozí platnou hodnotu při chybě přenosu
            }
            
            // Mrtvé pásmo pro eliminaci driftu v naprostém klidu
            if (std::abs(gz) < 0.015f) gz = 0.0f;
            
            // Integrace úhlové rychlosti (převod na stupně s korekcí měřítka)
            state.angleZ += gz * (180.0f / M_PI) * dt * GYRO_SCALE_CORRECTION;
        } else {
            if (task_loop_count % 1000 == 0) {
                Serial.println("[gyroTask] ERROR: Sensor read failed!");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1)); // Zvýšení frekvence na 1000 Hz pro maximální přesnost
    }
}

inline void gyroInit() {
    auto &state = getGyroState();
    if (state.initialized) {
        Serial.println("[gyroInit] Gyro is already initialized.");
        return;
    }

    uint32_t t_wire = millis();
    Serial.println("[gyroInit] Initializing I2C Wire on pins 14 (SDA), 26 (SCL)...");
    Wire.begin(14, 26, 400000);
    Wire.setTimeOut(20);
    delay(100);
    Serial.printf("[gyroInit] -> Wire.begin a delay trval: %lu ms\n", (unsigned long)(millis() - t_wire));

    bool status = false;
    uint8_t detected_addr = 0;
    
    uint32_t t_detect = millis();
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
    Serial.printf("[gyroInit] -> Detekce adresy trvala: %lu ms\n", (unsigned long)(millis() - t_detect));

    if (!status) {
        Serial.println("=================================================================");
        Serial.println("CRITICAL ERROR: Failed to find MPU-6050 chip at 0x68 or 0x69!");
        Serial.println("Please check I2C connection, power supply and SDA/SCL pull-ups.");
        Serial.println("=================================================================");
        return;
    }

    Serial.printf("[gyroInit] MPU-6050 chip found successfully at address: 0x%02X\n", detected_addr);

    // TEST rychlosti surového I2C čtení z MPU6050
    {
        uint32_t t_raw_test = millis();
        Wire.beginTransmission(detected_addr);
        Wire.write(0x3B); // ACCEL_XOUT_H
        Wire.endTransmission(false);
        Wire.requestFrom((uint8_t)detected_addr, (uint8_t)14);
        uint8_t buf[14];
        for(int i=0; i<14 && Wire.available(); i++) {
            buf[i] = Wire.read();
        }
        Serial.printf("[gyroInit DIAGNOSTIKA] Surové Wire čtení 14 bajtů trvalo: %lu ms\n", (unsigned long)(millis() - t_raw_test));
    }

    uint32_t t_set = millis();
    // Nastavení parametrů senzoru (rozsah 2000 stupňů/s pro eliminaci saturace při rychlých pohybech)
    state.mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    state.mpu.setGyroRange(MPU6050_RANGE_2000_DEG);
    state.mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    delay(100);
    Serial.printf("[gyroInit] -> Nastavení parametrů a delay trvalo: %lu ms\n", (unsigned long)(millis() - t_set));

    Serial.println("=============================================");
    Serial.println("Calibrating MPU-6050... KEEP ROBOT STILL!");
    Serial.println("=============================================");

    uint32_t t_calib = millis();
    float sumZ = 0.0f;
    const int samples = 100;
    int successful_reads = 0;
    
    for (int i = 0; i < samples; i++) {
        sensors_event_t a, g, temp;
        uint32_t t_read = millis();
        bool ok = state.mpu.getEvent(&a, &g, &temp);
        uint32_t read_duration = millis() - t_read;
        if (ok) {
            successful_reads++;
            sumZ += g.gyro.z;
            if (i < 5) {
                Serial.printf("Sample %d (doba čtení %lu ms): raw_gyro_z = %.6f rad/s (%.3f deg/s), acc_z = %.3f m/s^2\n", 
                              i, (unsigned long)read_duration, g.gyro.z, g.gyro.z * 180.0f / M_PI, a.acceleration.z);
            }
        } else {
            Serial.printf("Sample %d (doba čtení %lu ms): READ FAILED!\n", i, (unsigned long)read_duration);
        }
        delay(10);
    }
    state.rawOffsetZ = sumZ / (successful_reads > 0 ? successful_reads : 1);
    state.angleZ = 0.0f;
    state.offsetZ = 0.0f;
    state.lastFilteredGyroZ = 0.0f;
    state.initialized = true;

    Serial.printf("MPU-6050 Calibration completed.\nSuccessful reads: %d/%d, Calculated rawOffsetZ: %.6f rad/s\n", 
                  successful_reads, samples, state.rawOffsetZ);
    Serial.printf("[gyroInit] -> Kalibrační smyčka (100 vzorků) trvala celkem: %lu ms\n", (unsigned long)(millis() - t_calib));
    Serial.println("=============================================");

    uint32_t t_task = millis();
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
    Serial.printf("[gyroInit] xTaskCreatePinnedToCore returned: %d (trvalo %lu ms)\n", task_created, (unsigned long)(millis() - t_task));
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
 * \brief Jízda rovně stabilizovaná gyroskopem k absolutnímu cílovému úhlu.
 * 
 * Rychlostní profil:
 *   1. ROZJEZD: Plynulé zrychlení z 0% na požadovanou rychlost (časová rampa)
 *   2. PLAVBA:  Konstantní rychlost s gyro korekcí směru
 *   3. BRZDĚNÍ: Plynulé zpomalení na dojezdovou rychlost (podle zbývající vzdálenosti)
 *   4. DOJEZD:  Přesné dojetí na cíl velmi nízkou rychlostí (crawl_speed)
 */
inline MoveResult move_straight_gyro(float mm, float speed, uint32_t timeout_ms = 5000, float target_heading = 0.0f) {
    if (mm == 0 || speed == 0) { return {true, 0.0f}; }

    bool reverse = (mm < 0);
    float encoder_to_real = 1965.0f / 1973.0f;
    float real_target_mm = std::abs(mm);

    // Vynulování odometrie
    rkMotorsSetPositionLeft(0);
    rkMotorsSetPositionRight(0);
    delay(50);

    // === PARAMETRY RAMP ===
    float abs_target_speed = std::abs(speed);
    if (abs_target_speed < 5.0f) abs_target_speed = 5.0f;
    if (abs_target_speed > 100.0f) abs_target_speed = 100.0f;

    float crawl_speed = 4.0f;            // Dojezdová rychlost (4 %) pro přesný dojezd
    float accel_time_ms = 600.0f;         // Doba rozjezdu z 0 na cílovou rychlost [ms]
    float decel_distance_mm = 80.0f;      // Vzdálenost pro zpomalení na crawl [mm]
    float crawl_distance_mm = 15.0f;      // Posledních 15 mm jedeme crawl rychlostí

    // Pokud je celková vzdálenost krátká, přizpůsobíme rampy
    if (real_target_mm < 100.0f) {
        decel_distance_mm = real_target_mm * 0.5f;
        crawl_distance_mm = real_target_mm * 0.2f;
        accel_time_ms = 300.0f;
    }

    float speed_sign = reverse ? -1.0f : 1.0f;
    float kp_gyro = 0.25f;               // P-regulátor pro korekci směru

    unsigned long start_time = millis();
    float current_speed = 0.0f;           // Aktuální rychlost (0–100 %)

    auto clamp_speed = [](float s) -> int8_t {
        if (s > 100.0f) return 100;
        if (s < -100.0f) return -100;
        return static_cast<int8_t>(s);
    };

    float avg_pos = 0.0f;
    int loop_counter = 0;

    while (true) {
        // Okamžité zastavení při detekci soupeře na pozadí
        extern volatile bool opponentDetected;
        if (opponentDetected) {
            rkMotorsSetSpeed(0, 0);
            
            // Zjistíme detailně, které senzory a jaké hodnoty vyvolaly zastavení
            uint32_t u1 = uz_predni();
            uint32_t u2 = uz_predni_levy();
            uint32_t u3 = uz_predni_pravy();
            
            Serial.println("[move_straight_gyro] ZASTAVENO: Detekován soupeř!");
            Serial.print("[ZASTAVENÍ DETAILED] Aktivní triggery: ");
            if (u1 > 30 && u1 < 300) {
                Serial.printf("U1 (predni): %u mm | ", u1);
            }
            if (u2 > 30 && u2 < 350) {
                Serial.printf("U2 (predni levy): %u mm | ", u2);
            }
            if (u3 > 30 && u3 < 350) {
                Serial.printf("U3 (predni pravy): %u mm | ", u3);
            }
            Serial.println();
            
            // Vrátíme false a doposud ujetou vzdálenost
            float pos_l = std::abs(rkMotorsGetPositionLeft(true));
            float pos_r = std::abs(rkMotorsGetPositionRight(false));
            float current_real_pos = ((pos_l + pos_r) / 2.0f) * encoder_to_real;
            return {false, reverse ? -current_real_pos : current_real_pos};
        }

        loop_counter++;
        unsigned long now = millis();
        float elapsed_ms = (float)(now - start_time);

        // Čtení enkodérů
        rb::Manager::get().motor(rk::gCtx.motors().idRight()).requestInfo(nullptr);
        float pos_l = std::abs(rkMotorsGetPositionLeft(true));
        float pos_r = std::abs(rkMotorsGetPositionRight(false));
        avg_pos = (pos_l + pos_r) / 2.0f;
        float real_pos = avg_pos * encoder_to_real;

        // Cíl dosažen
        if (real_pos >= real_target_mm) {
            break;
        }

        // Timeout
        if (elapsed_ms > (float)timeout_ms) {
            rkMotorsSetSpeed(0, 0);
            return {false, reverse ? -real_pos : real_pos};
        }

        float dist_remaining = real_target_mm - real_pos;

        // === VÝPOČET CÍLOVÉ RYCHLOSTI ===
        float desired_speed;

        if (dist_remaining <= crawl_distance_mm) {
            // FÁZE 4: DOJEZD — jedeme crawl rychlostí pro přesnost
            desired_speed = crawl_speed;
        } else if (dist_remaining <= decel_distance_mm) {
            // FÁZE 3: BRZDĚNÍ — lineární pokles z aktuální rychlosti na crawl
            float decel_progress = (dist_remaining - crawl_distance_mm) / (decel_distance_mm - crawl_distance_mm);
            desired_speed = crawl_speed + (abs_target_speed - crawl_speed) * decel_progress;
        } else if (elapsed_ms < accel_time_ms) {
            // FÁZE 1: ROZJEZD — plynulé zrychlení z 0 na cílovou rychlost (časová rampa)
            float accel_progress = elapsed_ms / accel_time_ms;
            // Použijeme S-křivku (smoothstep) pro plynulejší rozjezd bez cuknutí
            float smooth = accel_progress * accel_progress * (3.0f - 2.0f * accel_progress);
            desired_speed = crawl_speed + (abs_target_speed - crawl_speed) * smooth;
        } else {
            // FÁZE 2: PLAVBA — plná rychlost
            desired_speed = abs_target_speed;
        }

        // Omezení rychlosti změny (anti-jerk filtr)
        float max_speed_change = 2.0f; // max změna rychlosti za 1 iteraci (10ms)
        if (desired_speed > current_speed + max_speed_change) {
            current_speed += max_speed_change;
        } else if (desired_speed < current_speed - max_speed_change) {
            current_speed -= max_speed_change;
        } else {
            current_speed = desired_speed;
        }

        // Nikdy nejezdi pod crawl (pokud ještě nedobrzdil na 0)
        if (current_speed < crawl_speed) current_speed = crawl_speed;

        // === GYRO KOREKCE SMĚRU ===
        float heading = gyroGetAngleZ();
        float heading_error = heading - target_heading;
        float correction = heading_error * kp_gyro;
        if (reverse) {
            correction = -correction;
        }

        float speed_l = current_speed + correction;
        float speed_r = current_speed - correction;

        // Diagnostika
        // Diagnostika (včetně senzorů) každých 15 iterací (~150 ms)
        if (loop_counter % 15 == 0) {
            uint32_t u1 = uz_predni();
            uint32_t u2 = uz_predni_levy();
            uint32_t u3 = uz_predni_pravy();
            uint32_t u4 = uz_zadek();
            int laser = uz_laser();
            Serial.printf("[GyroDrive] Pos: %.1f/%.1f mm | Rem: %.1f | Speed: %.1f%% | U1:%u U2:%u U3:%u U4:%u | Laser: %d mm | Head: %.2f°\n", 
                          real_pos, real_target_mm, dist_remaining, current_speed, u1, u2, u3, u4, laser, heading);
        }

        rkMotorsSetSpeed(clamp_speed(speed_l * speed_sign), clamp_speed(speed_r * speed_sign));
        delay(10);
    }

    rkMotorsSetSpeed(0, 0);
    float real_traveled = avg_pos * encoder_to_real;
    
    Serial.printf("[GyroDrive] Finished: Traveled %.1f mm (target %.1f mm, error %.1f mm)\n",
                  real_traveled, real_target_mm, real_target_mm - real_traveled);
    
    return {true, reverse ? -real_traveled : real_traveled};
}

/**
 * \brief Otočení o zvolený absolutní úhel pomocí gyroskopu.
 * \param target_global_angle Cílový absolutní úhel ve stupních (0 = směr startu, kladný doleva, záporný doprava).
 */
inline void turn_gyro(float target_global_angle, float speed, uint32_t timeout_ms = 5000) {
    if (speed <= 0) return;

    rkMotorsSetSpeed(0, 0);
    delay(200);

    float start_angle = gyroGetAngleZ();
    float delta_angle = target_global_angle - start_angle;
    
    // Pokud je rozdíl zanedbatelný, netočme se
    if (std::abs(delta_angle) < 0.5f) return;

    float max_speed = std::abs(speed);
    if (max_speed > 15.0f) {
        max_speed = 15.0f; // Bezpečnostní limit rychlosti snížen na 15 pro přesnost
    }

    // Parametry rychlostního profilu
    float min_speed = 3.0f;
    float ramp_up_deg = 15.0f;
    float ramp_down_deg = 30.0f;
    
    // Dynamický offset: pro malé zatáčky zmenšíme offset, aby nedocházelo k předčasnému vypnutí
    float active_offset = 3.5f;
    float abs_delta = std::abs(delta_angle);
    if (abs_delta < 15.0f) {
        active_offset = abs_delta * 0.2f; // 20% celkového úhlu
        if (active_offset < 0.5f) active_offset = 0.5f;
    }

    bool turn_left = (delta_angle > 0);
    
    unsigned long start_time = millis();

    auto clamp_speed = [](float s) -> int8_t {
        if (s > 100.0f) return 100;
        if (s < -100.0f) return -100;
        return static_cast<int8_t>(s);
    };

    int loop_counter = 0;
    float last_check_angle = start_angle;
    int stall_counter = 0;
    float min_speed_boost = 0.0f;

    while (true) {
        loop_counter++;
        if (millis() - start_time > timeout_ms) break;

        float current_angle = gyroGetAngleZ();
        
        // Podmínka zastavení (jednosměrný pohyb k absolutnímu cíli s offsetem)
        if (turn_left) {
            if (current_angle >= target_global_angle - active_offset) {
                break;
            }
        } else {
            if (current_angle <= target_global_angle + active_offset) {
                break;
            }
        }

        float error = std::abs(target_global_angle - current_angle);
        float turned = std::abs(current_angle - start_angle);

        // Anti-stall kompenzace
        if (loop_counter % 10 == 0) {
            if (std::abs(current_angle - last_check_angle) < 0.08f) {
                stall_counter++;
                if (stall_counter >= 2) {
                    min_speed_boost += 0.5f;
                    if (min_speed_boost > 6.0f) min_speed_boost = 6.0f;
                }
            } else {
                stall_counter = 0;
                min_speed_boost = 0.0f;
            }
            last_check_angle = current_angle;
        }

        float active_min_speed = min_speed + min_speed_boost;

        // Rozjezdová rampa
        float speed_accel = max_speed;
        if (turned < ramp_up_deg) {
            speed_accel = active_min_speed + (max_speed - active_min_speed) * (turned / ramp_up_deg);
        }

        // Brzdná rampa
        float speed_decel = max_speed;
        if (error < ramp_down_deg) {
            if (error >= 7.0f) {
                speed_decel = active_min_speed + (max_speed - active_min_speed) * ((error - 7.0f) / 23.0f);
            } else {
                speed_decel = active_min_speed;
            }
        }

        float curr_speed = std::min(speed_accel, speed_decel);

        if (loop_counter % 4 == 0) {
            Serial.printf("[GyroTurn] Angle: %.2f deg | Target: %.2f deg | Err: %.2f | Speed: %.1f\n", 
                          current_angle, target_global_angle, target_global_angle - current_angle, curr_speed);
        }

        if (turn_left) {
            rkMotorsSetSpeed(clamp_speed(-curr_speed), clamp_speed(curr_speed));
        } else {
            rkMotorsSetSpeed(clamp_speed(curr_speed), clamp_speed(-curr_speed));
        }

        delay(5);
    }

    rkMotorsSetSpeed(0, 0);
    delay(150);
    float final_angle = gyroGetAngleZ();
    Serial.printf("[GyroTurn] Finished: Final angle: %.2f deg (target was: %.2f deg, real error: %.2f deg)\n", 
                  final_angle, target_global_angle, target_global_angle - final_angle);
}

/**
 * \brief Dorovnání se na cílový globální úhel gyroskopu s blikáním LED.
 */
inline void align_gyro(float target_global_angle, float speed, uint32_t timeout_ms = 3000) {
    unsigned long start_time = millis();

    auto clamp_speed = [](float s) -> int8_t {
        if (s > 100.0f) return 100;
        if (s < -100.0f) return -100;
        return static_cast<int8_t>(s);
    };

    int loop_count = 0;
    bool led_state = false;

    while (millis() - start_time < timeout_ms) {
        float current_angle = gyroGetAngleZ();
        float error = target_global_angle - current_angle;
        float abs_error = std::abs(error);

        // Limit dorovnání zvýšen na 0.8 stupně
        if (abs_error < 0.8f) {
            break;
        }

        // Blikání zelenou LED během dorovnávání (změna stavu každých 100 ms)
        loop_count++;
        if (loop_count % 10 == 0) {
            led_state = !led_state;
            rkLedGreen(led_state);
        }

        // Rychlost dorovnávání je VŽDY konstantně 2 % (pro maximální přesnost a klidný pohyb)
        float target_speed = (error > 0) ? 2.0f : -2.0f;

        rkMotorsSetSpeed(clamp_speed(-target_speed), clamp_speed(target_speed));
        delay(10);
    }
    rkMotorsSetSpeed(0, 0);
    rkLedGreen(false); // Ujistíme se, že LED po skončení nesvítí
}
    


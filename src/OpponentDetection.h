#pragma once

#include <Arduino.h>
#include "Movement.h"

// Deklarace funkce bzučáku z knihovny Robotka
void rkBuzzerSet(bool on);

// Globální proměnná označující detekci soupeře (true = soupeř detekován, false = volno)
extern volatile bool opponentDetected;

// Task pro periodické měření a detekci
inline void opponentTask(void *pvParameters) {
    int consecutive_detections = 0;
    
    // Používáme periodu 30 ms pro detekci soupeře (rychlejší odezva)
    const TickType_t xDelay = pdMS_TO_TICKS(30);
    
    while (true) {
        // Změříme 3 přední senzory
        uint32_t u1 = uz_predni();       // U1 (přední)
        uint32_t u2 = uz_predni_levy();  // U2 (přední levý)
        uint32_t u3 = uz_predni_pravy(); // U3 (přední pravý)
        
        bool detection_this_cycle = false;
        
        // Limity soupeře:
        // U1 (přední): 35 cm (350 mm)
        // U2, U3 (levý, pravý): 35 cm (350 mm)
        // Zároveň hodnota musí být větší než 3 cm (30 mm), abychom odfiltrovali nuly a chyby.
        
        // 1) Kontrola předního senzoru (U1)
        if (u1 > 30 && u1 < 350) {
            detection_this_cycle = true;
        }
        // 2) Kontrola předního levého senzoru (U2)
        else if (u2 > 30 && u2 < 350) {
            detection_this_cycle = true;
        }
        // 3) Kontrola předního pravého senzoru (U3)
        else if (u3 > 30 && u3 < 350) {
            detection_this_cycle = true;
        }
        
        if (detection_this_cycle) {
            consecutive_detections++;
            if (consecutive_detections >= 2) {
                if (!opponentDetected) {
                    opponentDetected = true;
                    Serial.printf("[OPPONENT] DETECTED! U1: %u mm, U2: %u mm, U3: %u mm\n", u1, u2, u3);
                }
                consecutive_detections = 2; // Omezení růstu
            }
        } else {
            consecutive_detections = 0;
            if (opponentDetected) {
                opponentDetected = false;
                Serial.println("[OPPONENT] Clear.");
            }
        }
        
        // Jednorázové zapípání při detekci soupeře (pouze na náběžnou hranu)
        static bool last_opponent_detected = false;
        static uint32_t beep_stop_time = 0;
        
        if (opponentDetected && !last_opponent_detected) {
            rkBuzzerSet(true);
            beep_stop_time = millis() + 200; // Pípáme po dobu 200 ms
        }
        
        if (beep_stop_time > 0 && millis() >= beep_stop_time) {
            rkBuzzerSet(false);
            beep_stop_time = 0;
        }
        
        last_opponent_detected = opponentDetected;
        
        vTaskDelay(xDelay);
    }
}

inline void opponentDetectionInit() {
    Serial.println("[OpponentDetection] Initializing opponent detection task...");
    
    // Spustíme task na jádře 1 s prioritou 1 (podobně jako gyroTask)
    xTaskCreatePinnedToCore(
        opponentTask,
        "opponentTask",
        4096,
        NULL,
        1,
        NULL,
        1
    );
}

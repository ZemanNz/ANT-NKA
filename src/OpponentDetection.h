#pragma once

#include <Arduino.h>
#include "Movement.h"

// Globální proměnná označující detekci soupeře (true = soupeř detekován, false = volno)
extern volatile bool opponentDetected;

// Task pro periodické měření a detekci
inline void opponentTask(void *pvParameters) {
    int consecutive_detections = 0;
    
    // Používáme periodu 200 ms pro detekci soupeře.
    const TickType_t xDelay = pdMS_TO_TICKS(200);
    
    while (true) {
        // Změříme 3 přední senzory
        uint32_t u1 = uz_predni();       // U1 (přední)
        uint32_t u2 = uz_predni_levy();  // U2 (přední levý)
        uint32_t u3 = uz_predni_pravy(); // U3 (přední pravý)
        
        bool detection_this_cycle = false;
        
        // Limity soupeře:
        // U1 (přední): 30 cm (300 mm)
        // U2, U3 (levý, pravý): 35 cm (350 mm)
        // Zároveň hodnota musí být větší než 3 cm (30 mm), abychom odfiltrovali nuly a chyby.
        
        // 1) Kontrola předního senzoru (U1)
        if (u1 > 30 && u1 < 300) {
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
            if (consecutive_detections >= 3) {
                if (!opponentDetected) {
                    opponentDetected = true;
                    Serial.printf("[OPPONENT] DETECTED! U1: %u mm, U2: %u mm, U3: %u mm\n", u1, u2, u3);
                }
                consecutive_detections = 3; // Omezení růstu
            }
        } else {
            consecutive_detections = 0;
            if (opponentDetected) {
                opponentDetected = false;
                Serial.println("[OPPONENT] Clear.");
            }
        }
        
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

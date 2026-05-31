#include "robotka.h"
#include <Arduino.h>
#include <string>

void clear() {
    rb::Manager::get().oled().fill(rb::Oled::Black);
    rb::Manager::get().oled().updateScreen();
}

void waitToNextTest() {
    delay(3000);
    clear();
}

void setup() {
    printf("RB3204-RBCX s Robotkou\n");

    // Inicializace knihovny Robotka
    rkConfig cfg;
    rkSetup(cfg);

    rkLedRed(true);

    printf("Inicializace MPU... (S ROBOTEM NEHYBAT!)\n");
    rkMpuInit();
    printf("MPU Inicializovano!\n");

    uint32_t startTime = millis();
    while (millis() - startTime < 10000) {
        printf("MPU - angle: X: %2.2f Y: %2.2f Z: %2.2f\n", rkMpuGetAngleX(), rkMpuGetAngleY(), rkMpuGetAngleZ());
        
        if (rkButtonIsPressed(BTN_UP)) {
            rkMpuResetZ();
            printf("--- RESET Z (stisknuto BTN_UP) ---\n");
        }
        
        delay(100);
    }

    printf("10 sekund ubehlo, zhasinam vypis a zacinam blikat LEDkama!\n");
    while (true) {
        // Blikáme červenou, žlutou a zelenou LED (modrá fyzicky nefunguje)
        rkLedRed(true);
        rkLedYellow(true);
        rkLedGreen(true);
        delay(500);
        
        rkLedRed(false);
        rkLedYellow(false);
        rkLedGreen(false);
        delay(500);
    }
}

void loop() {}
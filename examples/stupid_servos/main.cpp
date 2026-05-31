#include "robotka.h"

void setup() {
    Serial.begin(115200);
    rkConfig cfg;
    rkSetup(cfg);

    // Nastavení serva
    rkServosSetPosition(1, 90); // Servo 1 nastaví na 90°
    delay(3000);

    rkServosSetPosition(1, 0); // Servo 1 nastaví na 0°
    delay(3000);

    rkServosSetPosition(1, -90); // Servo 1 nastaví na 180°
    delay(3000);
}

void loop() {

    if (rkButtonIsPressed(BTN_UP)) {
        rkServosSetPosition(1, 0); // Servo 1 nastaví na 90°
    }
    if (rkButtonIsPressed(BTN_DOWN)) {
        rkServosSetPosition(1, 90); // Servo 1 nastaví na 180°
    }
    if (rkButtonIsPressed(BTN_LEFT)) {
        rkServosSetPosition(1, -90); // Servo 1 nastaví na 180°
    }
}
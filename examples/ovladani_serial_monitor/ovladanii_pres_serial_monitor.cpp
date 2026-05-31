#include "robotka.h"
#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    rkConfig cfg;
    rkSetup(cfg);
    
    Serial.println("Vyber režim:");
    Serial.println("1 - Tlačítka (default)");
    Serial.println("2 - Serial terminal (BTN_LEFT)");
}

void loop() {
    // Režim tlačítek
    if (rkButtonIsPressed(BTN_ON)) {
        forward(1000, 50);
    }
    else if (rkButtonIsPressed(BTN_RIGHT)) {
        backward(800, 40);
    }
    // Přepnutí do serial terminálu
    else if (rkButtonIsPressed(BTN_LEFT)) {
        Serial.println("Přepínám do serial terminálu...");
        rkSerialTerminal();
    }
    
    delay(100);
}
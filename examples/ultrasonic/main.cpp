#include "robotka.h"

void setup() {
    Serial.begin(115200);
    Serial.println("RB3204-RBCX");
    rkConfig cfg;
    rkSetup(cfg);
}

void loop() {
    // Čtení vzdálenosti z ultrazvukového senzoru
    int distance = rkUltraMeasure(0);
    Serial.printf("Ultrasonic distance: %d mm\n", distance); // vzdalenost je v mm !!!!!
    delay(500);
}
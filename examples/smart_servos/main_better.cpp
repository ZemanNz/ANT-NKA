#include <Arduino.h>
#include "robotka.h"

void setup() {
    rkConfig cfg;
    rkSetup(cfg);
    printf("Robotka started!\n");
    
    rkLedRed(true); // Turn on red LED
    rkLedBlue(true); // Turn on blue LED


    rkSmartServoInit(0, 0, 240, 500, 3);
    rkSmartServoInit(1, 0, 100);
    
    
}
void loop() {
    if (rkButtonIsPressed(BTN_UP)) {
        rkSmartServoMove(0,200);
        rkSmartServoSoftMove(1,50);
    }
    if (rkButtonIsPressed(BTN_DOWN)) {
        rkSmartServoMove(0,20);
        rkSmartServoSoftMove(1,0);
    } 
}
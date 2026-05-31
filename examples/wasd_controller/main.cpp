#include <Arduino.h>
#include "robotka.h"

void setup() {
    rkConfig cfg;
    rkSetup(cfg);
    printf("Robotka started!\n");
    
    rkLedRed(true); // Turn on red LED
    rkLedBlue(true); // Turn on blue LED

    wifi_control_wasd(); // pouzor at jsme pripojeni na stejnou wifi, v robotcu mit povoleny enable_wifi_control_wasd=true ave vysilaci spravnou ip adresu
    
}
void loop() {

}
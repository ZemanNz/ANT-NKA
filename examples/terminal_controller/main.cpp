#include <Arduino.h>
#include "robotka.h"

void setup() {
    rkConfig cfg;
    rkSetup(cfg);
    printf("Robotka started!\n");
    
    rkLedRed(true); // Turn on red LED
    rkLedBlue(true); // Turn on blue LED


    wifi_control_terminal(); // pozor at jsme pripojeni na stejnou wifi, v robotka.h mit povoleny enable_wifi_terminal=true a ve vysilaci spravnou ip adresu
    
}
void loop() {

}
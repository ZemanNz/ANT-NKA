#include <Arduino.h>
#include "robotka.h"

// Nejjednodušší struktura
typedef struct __attribute__((packed)) {
    uint8_t servo_id;
    uint8_t position; // 0-255
} SimpleCommand;

SimpleCommand cmd = {1, 128}; // Servo ID 1, střední pozice

void setup() {
    rkConfig cfg;
    rkSetup(cfg);
    printf("Robotka started!\n");
    
    rkLedRed(true); // Turn on red LED
    rkLedBlue(true); // Turn on blue LED

    delay(2000); // Wait for 2 seconds
    rkUartInit();

    delay(100); // Short delay to ensure UART is initialized
    printf("posilam data...\n");
    rkUartSend(&cmd, sizeof(cmd));
    
}
int start_mil = millis();
void loop() {
    
    if(millis() - start_mil > 3000) {
        start_mil = millis();
        printf("posilam data...\n");
        rkUartSend(&cmd, sizeof(cmd));
    }
    if(rkUartReceive(&cmd, sizeof(cmd))) {
        // TADY PRACUJEME S PŘIJATÝMI DATY:
        
        // 1. Výpis na serial
        printf("Servo %d -> Position %d\n", cmd.servo_id, cmd.position);

    }
}
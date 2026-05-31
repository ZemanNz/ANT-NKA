#include <Arduino.h>

// Stejná struktura jako na Robotce
typedef struct __attribute__((packed)) {
    uint8_t servo_id;
    uint8_t position; // 0-255
} SimpleCommand;

// Nastavení UART
#define UART_TX_PIN 17
#define UART_RX_PIN 16

SimpleCommand receivedCmd;

// Funkce pro odeslání zprávy
void uartSend(const void* data, size_t size) {
    const uint8_t SYNC0 = 0xAA;
    const uint8_t SYNC1 = 0x55;
    
    Serial2.write(SYNC0);
    Serial2.write(SYNC1);
    Serial2.write((const uint8_t*)data, size);
    
    Serial.printf("Odesláno: %d bytů\n", size);
}

// Funkce pro přijetí zprávy
bool uartReceive(void* data, size_t size) {
    const uint8_t SYNC0 = 0xAA;
    const uint8_t SYNC1 = 0x55;
    
    static enum { WAIT_SYNC0, WAIT_SYNC1, READ_PAYLOAD } state = WAIT_SYNC0;
    static size_t bytesRead = 0;
    static uint8_t* buffer = (uint8_t*)data;
    
    while (Serial2.available()) {
        uint8_t c = Serial2.read();
        
        switch (state) {
            case WAIT_SYNC0:
                if (c == SYNC0) state = WAIT_SYNC1;
                break;
                
            case WAIT_SYNC1:
                if (c == SYNC1) {
                    state = READ_PAYLOAD;
                    bytesRead = 0;
                } else {
                    state = (c == SYNC0) ? WAIT_SYNC1 : WAIT_SYNC0;
                }
                break;
                
            case READ_PAYLOAD:
                buffer[bytesRead++] = c;
                if (bytesRead >= size) {
                    state = WAIT_SYNC0;
                    return true;
                }
                break;
        }
    }
    return false;
}

void setup() {
    Serial.begin(115200);
    while(!Serial){delay(10);}
    delay(1000); // Čas na inicializaci sériové komunikace
    Serial2.begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
    
    Serial.println("ESP32 UART Communicator - Přijímací režim");
    Serial.println("Připraven k přijímání zpráv od Robotky...");
    
    // Odeslání testovací zprávy na začátku
    SimpleCommand testCmd = {1, 128};
    uartSend(&testCmd, sizeof(testCmd));
    Serial.println("Odeslán testovací příkaz: Servo 1 -> Pozice 128");
}

void loop() {
    // Hlavní smyčka - pouze přijímáme zprávy
    
    if (uartReceive(&receivedCmd, sizeof(receivedCmd))) {
        Serial.printf("Přijato: Servo ID=%d, Pozice=%d\n", 
                     receivedCmd.servo_id, receivedCmd.position);
        
        // Zde můžeš reagovat na přijatá data
        // Například ovládat něco na základě přijatého příkazu
    }
    
    delay(10); // Krátké zpoždění pro snížení zátěže CPU
}#include <Arduino.h>
#include "robotka.h"

// Nejjednodušší struktura
typedef struct __attribute__((packed)) {
    uint8_t servo_id;
    uint8_t position; // 0-255
} SimpleCommand;

void setup() {
    rkConfig cfg;
    rkSetup(cfg);
    rkUartInit(115200, 16, 17);
    
    printf("Simple UART receiver ready\n");
}

void loop() {
    SimpleCommand cmd;
    
    // Čekáme na příkaz
    if (rkUartReceive_blocking(&cmd, sizeof(cmd), 1000)) {
        // TADY PRACUJEME S PŘIJATÝMI DATY:
        
        // 1. Výpis na serial
        printf("Servo %d -> Position %d\n", cmd.servo_id, cmd.position);
        
        // 2. Ovládání hardware
        int angle = map(cmd.position, 0, 255, 0, 240);
        rkSmartServoMove(cmd.servo_id, angle, 100);
        
        // 3. Odeslání odpovědi
        rkUartSend(&cmd, sizeof(cmd));
    } else {
        printf("Timeout - žádná data\n");
    }
}
#include <Arduino.h>
#include "robotka.h"

// Struktura pro přijímání dat o senzorech
// Musí být stejná jako na odesílajícím ESP32
typedef struct __attribute__((packed)) {
    uint8_t sensor_id;
    uint16_t distance; // mm
} SensorData;

SensorData received_data;

void setup() {
    rkConfig cfg;
    rkSetup(cfg);
    printf("Robotka prijimac UART started!\n");
    
    // Inicializace UART komunikace
    rkUartInit();

    printf("Cekam na data ze senzoru...\n");
}

void loop() {
    // Zkusíme přijmout data
    if (rkUartReceive(&received_data, sizeof(received_data))) {
        // Pokud jsme úspěšně přijali data, vypíšeme je
        printf("Senzor ID: %d, Vzdalenost: %d mm\n", received_data.sensor_id, received_data.distance);
        if(received_data.sensor_id == 3){
          printf("-------------------------------------------------------- \n");
          std::cout<<" "<<std::endl;
        }
    }

    // Malá pauza, abychom nezahltili procesor
    delay(10); 
}


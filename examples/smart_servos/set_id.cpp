#include <Arduino.h>
#include "SmartServoBus.hpp"

using namespace lx16a;

static int n = 0;
static SmartServoBus servoBus;

void setup() {
    // Servos on the bus must have sequential IDs, starting from 0 (not 1)!
    servoBus.begin(1, UART_NUM_2, GPIO_NUM_14);

    // Set servo Id (must be only one servo connected to the bus)
    servoBus.setId(0); //nastavení serva na ID = 0
    while (true) {
        printf("GetId: %d\n", servoBus.getId()); // servo bude vypisovat jaky je id
        delay(1000);
    }
    
}
void loop() { // pro zkouzku pohybu....

    servoBus.set(0, Angle::deg(0));

    delay(5000);

    servoBus.set(0,Angle::deg(240));
    delay(5000);
}

#include "robotka.h"
#include "Arduino.h"

void setup() {
    Serial.begin(115200);

    // Inicializace robotky
    rkConfig cfg;
    rkSetup(cfg);

    Serial.println("All motor functions example started!");
    Serial.println("Each button executes one function:");
    Serial.println("BTN_ON: Sequence 1 - forward movements");
    Serial.println("BTN_RIGHT: Sequence 2 - backward movements"); 
    Serial.println("BTN_UP: Sequence 3 - turning movements");
    Serial.println("BTN_DOWN: Sequence 4 - radius movements");
    Serial.println("BTN_LEFT: Sequence 5 - special functions");
}

void loop() {
    // BTN_ON - Sekvence pohybů vpřed
    if (rkButtonIsPressed(BTN_ON)) {
        Serial.println("=== SEQUENCE 1: FORWARD MOVEMENTS ===");
        
        Serial.println("1. forward(1000, 50)");
        rkLedRed(true);
        forward(1000, 50);
        rkLedRed(false);
        delay(1000);
        
        Serial.println("2. forward_acc(800, 50)");
        rkLedGreen(true);
        forward_acc(800, 50);
        rkLedGreen(false);
        
        Serial.println("Sequence 1 completed!");
    }
    
    // BTN_RIGHT - Sekvence pohybů vzad
    else if (rkButtonIsPressed(BTN_RIGHT)) {
        Serial.println("=== SEQUENCE 2: BACKWARD MOVEMENTS ===");
        
        Serial.println("1. backward(800, 40)");
        rkLedBlue(true);
        backward(800, 40);
        rkLedBlue(false);
        delay(1000);
        
        Serial.println("2. backward_acc(600, 40)");
        rkLedYellow(true);
        backward_acc(600, 40);
        rkLedYellow(false);
        
        Serial.println("Sequence 2 completed!");
    }
    
    // BTN_UP - Sekvence otáčení na místě
    else if (rkButtonIsPressed(BTN_UP)) {
        Serial.println("=== SEQUENCE 3: TURNING MOVEMENTS ===");
        
        Serial.println("1. turn_on_spot_left(90, 40)");
        rkLedRed(true);
        turn_on_spot_left(90, 40);
        rkLedRed(false);
        delay(1000);
        
        Serial.println("2. turn_on_spot_right(90, 40)");
        rkLedGreen(true);
        turn_on_spot_right(90, 40);
        rkLedGreen(false);
        delay(1000);
        
        Serial.println("3. turn_on_spot_left(180, 50)");
        rkLedBlue(true);
        turn_on_spot_left(180, 50);
        rkLedBlue(false);
        
        Serial.println("Sequence 3 completed!");
    }
    
    // BTN_DOWN - Sekvence otáčení s poloměrem
    else if (rkButtonIsPressed(BTN_DOWN)) {
        Serial.println("=== SEQUENCE 4: RADIUS MOVEMENTS ===");
        
        Serial.println("1. radius_left(200, 90, 40)");
        rkLedRed(true);
        radius_left(200, 90, 40);
        rkLedRed(false);
        delay(1000);
        
        Serial.println("2. radius_right(200, 90, 40)");
        rkLedGreen(true);
        radius_right(200, 90, 40);
        rkLedGreen(false);
        delay(1000);
        
        Serial.println("3. radius_left(150, 180, 50)");
        rkLedBlue(true);
        radius_left(150, 180, 50);

        front_buttons(40, []{ return rkButton1(); }, []{ return rkButton2(); });
        rkLedBlue(false);
        
        Serial.println("Sequence 4 completed!");
    }
    
    // BTN_LEFT - Speciální funkce
    else if (rkButtonIsPressed(BTN_LEFT)) {
        Serial.println("=== SEQUENCE 5: SPECIAL FUNCTIONS ===");
        
        Serial.println("1. back_buttons(30)");
        Serial.println("Couvání dokud nenarazí na zeď...");
        rkLedAll(true);
        back_buttons(40);
        rkLedAll(false);
        delay(1000);
        
        Serial.println("2. forward(500, 60) - návrat");
        rkLedGreen(true);
        forward(500, 60);
        rkLedGreen(false);
        delay(1000);
        orient_to_wall(true, []() -> uint32_t { return rkUltraMeasure(2); },
                             []() -> uint32_t { return rkUltraMeasure(1); });
                             
        delay(1000);
        wall_following(1300 ,30.0f, true,  100.0f, true,
                   []() -> uint32_t { return rkUltraMeasure(2); },
                   []() -> uint32_t { return rkUltraMeasure(1); }, -23); 
        
        Serial.println("Sequence 5 completed!");
    }
    
    delay(100);
}
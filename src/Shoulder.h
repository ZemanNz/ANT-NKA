#pragma once

#include "robotka.h"

//OVLÁDÍNÍ RAMENE

class Shoulder {
private:

    float rDefaultSpeed; // Výchozí rychlost pro serva

public:
    int iLeft;  // Pozice pro levou stranu
    int iRight; // Pozice pro pravou stranu

    Shoulder() : rDefaultSpeed(80), iLeft(35), iRight(215) {}

    void fSetDefaultSmartServosSpeed(float rSpeed)  { rDefaultSpeed = rSpeed; }

    int iGetDefaultSmartServosSpeed() const{ return rDefaultSpeed; }

    // Ovládíní Grabberu Baterie
    void Magnet(bool on)                    { if (on) { rkServosSetPosition(1, 90); } else { rkServosSetPosition(1, 0); } }    // 90 - drží baterii, 0 - pustí baterii
    
    // Pomocná metoda pro čekání na dojezd chytrého serva na požadovaný úhel
    void wait_for_servo(int id, int target_angle, uint32_t timeout_ms = 2500) {
        unsigned long start_time = millis();
        while (millis() - start_time < timeout_ms) {
            int current_pos = rkSmartServosPosicion(id);
            if (current_pos > 0 && current_pos < 255) {
                if (std::abs(current_pos - target_angle) <= 3) {
                    break;
                }
            }
            delay(20);
        }
        delay(500); // Pauza pro zklidnění baterie po dokončení pohybu
    }

    // Rameno - nahoře
    void Up(float rSpeed = -1)               { rkSmartServoMove(0, 65, rSpeed < 0 ? rDefaultSpeed : rSpeed); wait_for_servo(0, 65); }        

    // Rameno - dole
    void Down(float rSpeed = -1)             { rkSmartServoMove(0, 130, rSpeed < 0 ? rDefaultSpeed : rSpeed); wait_for_servo(0, 130); }       

    // Rameno - dole při vyložení (upraveno na 95 stupňů pro šetrnější položení)
    void DownUnload(float rSpeed = -1)       { rkSmartServoMove(0, 95, rSpeed < 0 ? rDefaultSpeed : rSpeed); wait_for_servo(0, 95); }      

    // Rameno - nahoře - aktiv
    void Active(float rSpeed = -1)           { rkSmartServoMove(0, 85, rSpeed < 0 ? rDefaultSpeed : rSpeed); wait_for_servo(0, 85); }        

    // Rameno - pohyb do strany (využívá proměnné iLeft nebo iRight)
    void Side(int iPos, float rSpeed = -1)   { rkSmartServoMove(1, iPos, rSpeed < 0 ? rDefaultSpeed : rSpeed); wait_for_servo(1, iPos); }        

    // Když bereme baterii ze strany
    void SideTakeBattery(int iPos, float rSpeed = -1) { 
        int iOffset = (iPos < 125) ? 10 : -10; // Levá přičítá 10, pravá odčítá 10
        rkSmartServoMove(1, iPos + iOffset, rSpeed < 0 ? rDefaultSpeed : rSpeed); 
        wait_for_servo(1, iPos + iOffset);
    } 

    // Pohyb ramenem lehce do strany aby se dorovnaly nepřesnosti
    void SideTolerance(int iPos, float rSpeed = -1) {
        float rS = rSpeed < 0 ? rDefaultSpeed : rSpeed;
        int iOffset = (iPos < 125) ? 10 : -10; // Tolerance nejdřív k robotovi a pak od něj
        
        rkSmartServoMove(1, iPos + iOffset, rS/2);
        delay(1000);
        rkSmartServoMove(1, iPos - iOffset, rS/2);
        delay(1000);
    }

    // Rameno - Střed - kouká za robota
    void Center(float rSpeed = -1)           { rkSmartServoMove(1, 125, rSpeed < 0 ? rDefaultSpeed : rSpeed); wait_for_servo(1, 125); }  

    /**
     * \brief Detekuje, zda rameno drží baterii (pomocí laserového ToF senzoru).
     *        Měří vícekrát (cílí na 3 platná měření) a průměruje je.
     *        Hodnoty <= 20 mm (včetně 0 a -1) jsou ignorovány jako chyby.
     * \return true pokud je průměrná vzdálenost menší než 80 mm.
     */
    bool HasBattery() {
        extern int uz_laser();
        int valid_count = 0;
        
        // Provedeme 5 rychlých měření
        for (int i = 0; i < 5; i++) {
            int dist = uz_laser();
            // Validní vzdálenost baterie na magnetu je v rozsahu 20 až 150 mm
            if (dist > 20 && dist < 150) {
                valid_count++;
            }
            delay(15); // Krátká pauza mezi měřeními
        }
        
        // Pokud alespoň 2 měření detekují překážku v tomto rozsahu, baterii máme
        return (valid_count >= 2);
    }

};

// Deklarace globální instance pro zbytek programu
extern Shoulder Rameno;
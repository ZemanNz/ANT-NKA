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
    
    // Rameno - nahoře
    void Up(float rSpeed = -1)               { rkSmartServoSoftMove(0, 60, rSpeed < 0 ? rDefaultSpeed : rSpeed); }        

    // Rameno - dole
    void Down(float rSpeed = -1)             { rkSmartServoSoftMove(0, 130, rSpeed < 0 ? rDefaultSpeed : rSpeed); }       

    // Rameno - dole při vyložení
    void DownUnload(float rSpeed = -1)       { rkSmartServoSoftMove(0, 100, rSpeed < 0 ? rDefaultSpeed : rSpeed); }      

    // Rameno - nahoře - aktiv
    void Active(float rSpeed = -1)           { rkSmartServoSoftMove(0, 85, rSpeed < 0 ? rDefaultSpeed : rSpeed); }        

    // Rameno - pohyb do strany (využívá proměnné iLeft nebo iRight)
    void Side(int iPos, float rSpeed = -1)   { rkSmartServoSoftMove(1, iPos, rSpeed < 0 ? rDefaultSpeed : rSpeed); }        

    // Když bereme baterii ze strany
    void SideTakeBattery(int iPos, float rSpeed = -1) { 
        int iOffset = (iPos < 125) ? 10 : -10; // Levá přičítá 10, pravá odčítá 10
        rkSmartServoSoftMove(1, iPos + iOffset, rSpeed < 0 ? rDefaultSpeed : rSpeed); 
    } 

    // Pohyb ramenem lehce do strany aby se dorovnaly nepřesnosti
    void SideTolerance(int iPos, float rSpeed = -1) {
        float rS = rSpeed < 0 ? rDefaultSpeed : rSpeed;
        int iOffset = (iPos < 125) ? 10 : -10; // Tolerance nejdřív k robotovi a pak od něj
        
        rkSmartServoSoftMove(1, iPos + iOffset, rS/2);
        delay(1000);
        rkSmartServoSoftMove(1, iPos - iOffset, rS/2);
        delay(1000);
    }

    // Rameno - Střed - kouká za robota
    void Center(float rSpeed = -1)           { rkSmartServoSoftMove(1, 125, rSpeed < 0 ? rDefaultSpeed : rSpeed); }  

};

// Deklarace globální instance pro zbytek programu
extern Shoulder Rameno;
#pragma once

#include <Arduino.h>

class ContestTimer {
private:
    TaskHandle_t taskHandle;
    unsigned long startMillis;
    volatile uint32_t iElapsedSeconds;
    volatile bool bRunning;

    // Samotný Task (vlákno) běžící na pozadí FreeRTOS
    static void timerTask(void* parameter) {
        ContestTimer* Timer = static_cast<ContestTimer*>(parameter);
        while (true) {
            if (Timer->bRunning) { Timer->iElapsedSeconds = (millis() - Timer->startMillis) / 1000; }
            // Vlákno se uspí na 500 ms, aby nezabíralo výkon procesoru
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }

public:
    ContestTimer() : taskHandle(nullptr), startMillis(0), iElapsedSeconds(0), bRunning(false) {}

    // Spustí časovač (a případně vytvoří vlákno, pokud ještě neexistuje)
    void fStart() {
        startMillis = millis();
        iElapsedSeconds = 0;
        bRunning = true;
        
        if (taskHandle == nullptr) {
            xTaskCreatePinnedToCore(
                timerTask,          // Funkce vlákna
                "ContestTimer",     // Název vlákna
                2048,               // Velikost paměti
                this,               // Parametr předaný vláknu (ukazatel na instanci)
                1,                  // Priorita
                &taskHandle,        // Handle vlákna
                0                   // Běh na jádře 0 (mimo hlavní Arduino loop)
            );
        }
    }

    // Zastaví časovač
    void fStop() { bRunning = false; }

    // Vrátí aktuálně uběhlý čas v sekundách
    uint32_t iGetElapsedSeconds() const { return iElapsedSeconds; }

    // Zjistí, zda nezbývá příliš málo času (limit_s = délka soutěže, tolerance_s = čas potřebný na návrat)
    bool bIsTimeRunningOut(uint32_t limit_s, uint32_t tolerance_s) const {
        if (!bRunning) return false;
        return iElapsedSeconds >= (limit_s - tolerance_s);
    }
};

// Deklarace globální instance (samotná definice je v main.cpp)
extern ContestTimer GameTimer;
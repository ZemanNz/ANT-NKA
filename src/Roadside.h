#pragma once // Zabrání vícenásobnému načtení souboru

#include <cstdio>
#include <vector>
#include <functional>
#include <cmath> // Přidáno pro matematickou funkci abs
#include "Movement.h"
#include "Shoulder.h"

// Definice barev týmů
enum class TeamColor {
    Blue,
    Red,
    Unknown
};

// Stavy elektromobilu (Docku)
enum class DockStatus {
    Empty,  // Naše barva, čeká na baterii
    Full,   // Úspěšně vložena baterie
    Enemy   // Barva soupeře
};

// Stavy baterie
enum class BatteryStatus {
    Available, 
    Taken      
};

// Struktura pro X, Y souřadnice v mm
struct Position {
    float fX;
    float fY;
};

// Struktura Docků: X, Y, BARVA, STAV
struct Dock {
    Position Pos;
    TeamColor Color;
    DockStatus Status;
};

// Struktura pro Baterii: X, Y, STAV
struct Battery {
    Position Pos;
    BatteryStatus Status;
};

// ============================================================

class GameManager {
private:
    std::vector<Dock> vDocks;
    std::vector<Battery> vBatteries;
    TeamColor MyColor;

    // Pomocná funkce pro vygenerování kontroly překážek na základě směru jízdy
    std::function<bool()> fGetObstacleChecker(float rDistanceToGo) {
        bool bForward = (rDistanceToGo > 0);
        return [this, bForward]() -> bool {
            uint32_t dist = rkUltraMeasure(bForward ? iUltraFront : iUltraBack);
            // Hodnota 0 znamená chybu měření (např. senzor nic nevidí nebo není zapojen), proto ignorujeme 0
            return (dist > 0 && dist <= iObstacleStopZoneMm);
        };
    }

public:
    // --- Nastavení Ultrazvuků (ID 1-4 podle portů na desce) ---
    uint8_t iUltraFront = 1;
    uint8_t iUltraBack  = 2;
    uint8_t iUltraLeft  = 3;
    uint8_t iUltraRight = 4;
    
    uint32_t iObstacleStopZoneMm = 150; // Zóna, ve které robot zastaví před překážkou (v mm)

    void fInitGame(int iButtonClicks, TeamColor ChosenColor) {
        std::vector<std::vector<TeamColor>> vAllLayouts = {
            // Kombinace 0: Naše auto (Blue) je 1. v pořadí zleva
            {TeamColor::Blue, TeamColor::Red, TeamColor::Red, TeamColor::Red,
             TeamColor::Blue, TeamColor::Blue, TeamColor::Blue, TeamColor::Red},

            // Kombinace 1: Naše auto (Blue) je 2. v pořadí zleva
            {TeamColor::Red, TeamColor::Blue, TeamColor::Red, TeamColor::Red,
             TeamColor::Blue, TeamColor::Blue, TeamColor::Red, TeamColor::Blue},

            // Kombinace 2: Naše auto (Blue) je 3. v pořadí zleva
            {TeamColor::Red, TeamColor::Red, TeamColor::Blue, TeamColor::Red,
             TeamColor::Blue, TeamColor::Red, TeamColor::Blue, TeamColor::Blue},

            // Kombinace 3: Naše auto (Blue) je 4. v pořadí zleva
            {TeamColor::Red, TeamColor::Red, TeamColor::Red, TeamColor::Blue,
             TeamColor::Red, TeamColor::Blue, TeamColor::Blue, TeamColor::Blue}
        };

        // Zabezpečení kliknutí a zacyklení
        int iSelectedLayoutIndex = iButtonClicks % 4;
        
        // Použití tlačítka ZPĚT (Navrácení záporné hodnoty [odčítáme: iButtonClicks])
        if (iSelectedLayoutIndex < 0) { iSelectedLayoutIndex += 4; }

        printf("--- INICIALIZACE HRY ---\n");
        printf("Pocet kliknuti: %d\n", iButtonClicks);
        printf("Vybrany index kombinace: %d\n", iSelectedLayoutIndex);

        // Vnitřní volání nastavovacích funkcí
        fSetupDocks(vAllLayouts[iSelectedLayoutIndex], ChosenColor);
        fSetupBatteries();
    }

    // =============================================
    // ---- NASTAVENÍ POZIC DOCŮ A BATERIÍ ZDE ----

    // Inicializace Docků na základě vylosovaného rozložení
    void fSetupDocks(const std::vector<TeamColor>& vDrawnLayout, TeamColor ChosenColor) {
        MyColor = ChosenColor;
        vDocks.clear(); // Pro jistotu vyčistíme předchozí data

        // Fyzické pozice 8 Docků na hřišti v mm
        float rFixedXPositions[8] = {180, 550, 920, 1300, 1700, 2080, 2450, 2820};
        float rFixedYPosition = 100.0f; 

        // Defaultní nastavení všech 8 Docků (Pozice|Stav|Barva)
        for (int i = 0; i < 8; ++i) {
            DockStatus CurrentStatus;
            
            if (vDrawnLayout[i] == MyColor) { CurrentStatus = DockStatus::Empty; }
            else { CurrentStatus = DockStatus::Enemy; }
            
            vDocks.push_back({{rFixedXPositions[i], rFixedYPosition}, vDrawnLayout[i], CurrentStatus}); 
        }
    }

    // Inicializace všech 8 Baterií (2 řady, 4 sloupce)
    void fSetupBatteries() {
        vBatteries.clear();
        
        // X souřadnice pro 4 sloupce (kolem středu 1500 mm)
        float rX_Positions[4] = {1300.0f, 1430.0f, 1570.0f, 1700.0f}; 
        
        // Y souřadnice pro 2 řady (kolem středu 1000 mm)
        float rY_Closer = 950.0f;   // Bližší k autům
        float rY_Farther = 1050.0f; // Vzdálenější od aut
        
        for (int i = 0; i < 4; ++i) {
            // Vložíme bližší baterii
            vBatteries.push_back({{rX_Positions[i], rY_Closer}, BatteryStatus::Available});
            // Vložíme vzdálenější baterii
            vBatteries.push_back({{rX_Positions[i], rY_Farther}, BatteryStatus::Available});
        }
    }

    // ============================================================
    // GETTERY (Získání konkrétní souřadnice X nebo Y Baterie / Docku podle indexu)
    // ============================================================

    float rGetDockPosX(int index) {
        if (index >= 0 && index < vDocks.size()) return vDocks[index].Pos.fX;
        return -1.0f; // Návratová hodnota při chybě
    }

    float rGetDockPosY(int index) {
        if (index >= 0 && index < vDocks.size()) return vDocks[index].Pos.fY;
        return -1.0f;
    }

    float rGetBatteryPosX(int index) {
        if (index >= 0 && index < vBatteries.size()) return vBatteries[index].Pos.fX;
        return -1.0f;
    }

    float rGetBatteryPosY(int index) {
        if (index >= 0 && index < vBatteries.size()) return vBatteries[index].Pos.fY;
        return -1.0f;
    }

    // ============================================================
    // HLEDÁNÍ NEJBLIŽŠÍCH CÍLŮ POUZE V OSE X
    // ============================================================

    // Najde index volné baterie, která je robotovi nejblíže v ose X
    int fFindClosestBattery(float fRobotX) {
        int iBestIndex = -1;
        float rMinDistance = 999999.0f;
        
        for (int i = 0; i < vBatteries.size(); ++i) {
            if (vBatteries[i].Status == BatteryStatus::Available) {
                float rDistX = std::abs(vBatteries[i].Pos.fX - fRobotX); // Počítá vzdálenost v ose x
                if (rDistX < rMinDistance) {
                    rMinDistance = rDistX;
                    iBestIndex = i;
                }
            }
        }
        return iBestIndex;
    }

    // Najde index volného Docku (naše barva, stav Empty), který je v ose X nejblíže
    int fFindClosestEmptyDock(float fRobotX) {
        int iBestIndex = -1;
        float rMinDistance = 999999.0f;
        
        for (int i = 0; i < vDocks.size(); ++i) {
            if (vDocks[i].Status == DockStatus::Empty) {
                
                float rDistX = std::abs(vDocks[i].Pos.fX - fRobotX); // Počítáme vzdálenost v ose X
                if (rDistX < rMinDistance) {
                    rMinDistance = rDistX;
                    iBestIndex = i;
                }
            }
        }
        return iBestIndex;
    }

    // Označí konkrétní baterii jako sebranou
    void fMarkBatteryTaken(int index) { if (index >= 0 && index < vBatteries.size()) { vBatteries[index].Status = BatteryStatus::Taken; } }

    // Označí konkrétní Dock jako plný (úspěšně vložena baterie)
    void fMarkDockFull(int index) { if (index >= 0 && index < vDocks.size()) { vDocks[index].Status = DockStatus::Full; } }

    // Zkontroluje, jestli už jsou všechny naše Docky plné
    bool bAreAllOurDocksFull() {
        for (const auto& dock : vDocks) { if (dock.Status == DockStatus::Empty) { return false; } }
        return true; // Žádný prázdný Dock už nezbývá
    }

    // ============================================================
    // DOPLŇOVÁNÍ DOCKŮ
    // ============================================================

    // Najde, dojede a sebere nejbližší baterii.
    void fTakeClosestBattery(float& fRobotX) {

        // Najití ID nejbližší baterie
        int iClosestBatteryID = fFindClosestBattery(fRobotX);
        
        if (iClosestBatteryID != -1) {

            // Získání pozice baterie
            float rX = rGetBatteryPosX(iClosestBatteryID);
            printf("Jedu k nejblizsi baterii na X = %.1f\n", rX);
            
            // Výpočet potřebné vzdálenosti na ujetí
            float rDistanceToGo = rX - fRobotX;
            
            // Jízda na místo s neustálou detekcí překážek
            MoveResult result = move_acc_avoid(rDistanceToGo, 60, fGetObstacleChecker(rDistanceToGo), 5000);
            
            // Aktualizace přesné X-ové pozice na hřišti (ať už dojel nebo ne)
            fRobotX += result.traveled_mm; 
            
            // Robot úspěšně dojel až do cíle k baterii bez předčasného přerušení
            if (result.success) {

                int iBasePos = (MyColor == TeamColor::Blue) ? Rameno.iLeft : Rameno.iRight;
                
                // Přesun manipulátoru nad baterii
                Rameno.Side(iBasePos);
                delay(1000);
                
                // Sjetí k baterii
                Rameno.Down();
                delay(1000);

                // Uchopení baterie
                Rameno.SideTolerance(iBasePos);
                delay(1000);

                // Zvednutí baterie
                Rameno.Up();
                delay(1000);

                // Přesun baterie na pozici vhodou k pohybu robota
                Rameno.Center();
                delay(1000);

                // Zapsis baterie jako sebraná
                fMarkBatteryTaken(iClosestBatteryID);
                
                printf("Baterie sebrana! Aktualni pozice robota v X je: %.1f\n", fRobotX);
                
            } else { printf("K baterii se nepodarilo dojet kvuli prekazce! Zastavil jsem na X = %.1f\n", fRobotX); }
        
        } else { printf("Zadna volna baterie k dispozici.\n"); }
    }

    // Najde, dojede a vyloží baterii do nejbližšího volného Docku. 
    void fFillClosestDock(float& fRobotX) {

        // Najití ID nejbližšího prázdného Docku
        int iClosestDockID = fFindClosestEmptyDock(fRobotX);
        
        if (iClosestDockID != -1) {

            // Získání pozice Docku
            float rX = rGetDockPosX(iClosestDockID);
            printf("Jedu k nejblizsimu prazdnemu Docku na X = %.1f\n", rX);
            
            // Výpočet potřebné vzdálenosti k ujetí
            float rDistanceToGo = rX - fRobotX;
            
            // Jízda na místo s neustálou detekcí překážek
            MoveResult result = move_acc_avoid(rDistanceToGo, 60, fGetObstacleChecker(rDistanceToGo), 5000);
            
            // Aktualizace přesné X-ové pozice na hřišti (ať už dojel nebo ne)
            fRobotX += result.traveled_mm; 
            
            // Robot úspěšně dojel až do cíle k Docku bez předčasného přerušení
            if (result.success) {

                // Rozhodnutí na jakou stranu se bude Manipulátor otáčet dle barvy Týmu
                int iBasePos = (MyColor == TeamColor::Blue) ? Rameno.iRight : Rameno.iLeft;
                
                // Pohyb Manipulátorem nad Dock
                Rameno.Side(iBasePos);
                delay(1000);
                
                // Sjetí k Docku
                Rameno.Down();
                delay(1000);

                // Puštění baterie
                Rameno.Magnet(false);
                delay(1000);

                // Zvednutí prázdného ramene
                Rameno.Up();
                delay(1000);

                // Přesun baterie na pozici vhodou k pohybu robota
                Rameno.Center();
                delay(1000);

                // Nachystání magnetu na další baterii
                Rameno.Magnet(true);
                delay(1000);

                // Zápis Docku jako plný, aby se k němu už příště nejezdilo
                fMarkDockFull(iClosestDockID);
                
                printf("Baterie vlozena do Docku! Aktualni pozice robota v X je: %.1f\n", fRobotX);
            
            } else { printf("K Docku se nepodarilo dojet kvuli prekazce! Zastavil jsem na X = %.1f\n", fRobotX); }
        
        } else { printf("Zadny prazdny Dock neni k dispozici.\n"); }
    }

    // ============================================================
    // ODTLÁČENÍ NÁKLAĎÁKU
    // ============================================================

    // Odtlačí náklaďák o zadanou vzdálenost v ose Y.
    // Automaticky aktualizuje Y-ovou pozici robota.
    void fPushTruckAway(float& rRobotY, float rDistanceY) {
        printf("Zacinam odtlacovat nakladak o %.1f mm v ose Y.\n", rDistanceY);

        // Jízda na danou vzdálenost (pomalá a silová)
        // POZOR: Tady záměrně detekci ZVYPNEME ([]() { return false; }), protože náklaďák JE překážka, kterou chceme nabourat!
        MoveResult result = move_acc_avoid(rDistanceY, 40, []() { return false; }, 8000);

        // Aktualizace přesné Y-ové pozice na hřišti
        rRobotY += result.traveled_mm;

        if (result.success) { printf("Nakladak uspesne odtlacen! Nova pozice robota v Y je: %.1f\n", rRobotY); }
        else { printf("Odtlaceni se nezdarilo! Zastavil jsem na Y = %.1f\n", rRobotY); }
    }



};
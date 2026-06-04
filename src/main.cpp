#include "robotka.h"

#include <Arduino.h>
#include <string>

#include "Roadside.h"
#include "Movement.h"
#include "movement_gyro.h"
#include "ContestTimer.h"
#include "Shoulder.h"


// Nastavení Roadsidu
GameManager RoadsideGame;

// Nastavení Časovače
ContestTimer GameTimer; 

// Globální instance Ramene (propojí se přes extern všude tam, kde je Shoulder.h) 
Shoulder Rameno;

// --- Proměnné pro MENU ---
enum class MenuState {
    ROADSIDE_SELECT_TEAM,
    ROADSIDE_SELECT_LAYOUT,
    ROADSIDE_WAIT_START,
    GAME_RUNNING
};

MenuState eCurrentState = MenuState::ROADSIDE_SELECT_TEAM;
TeamColor eSelectedTeam = TeamColor::Blue; // Výchozí tým
int iSelectedLayout = 0; 
bool bRoadsideGameStarted = false;


float rCurrentRobotX = 1400.0f; 
float rCurrentRobotY = 200.0f;  

int input_speed = 40;
float speed_forward_corr = 1;
float speed_backward_corr = 1;


int rameno_pos = 65;
int rDefSpeed = 80; 

void setup() {
    printf("robotka started\n");

    // Inicializace knihovny Robotka
    rkConfig cfg;

    //cfg.prevod_motoru = 1983.3f; // pro 12v ==  41.62486f * 48.f, pro 6v == 1981.3f
    cfg.prevod_motoru = 889.0f; // Finální kalibrace: ujel 970 mm misto 1000 mm -> (1000/970)*862.3
    cfg.left_wheel_diameter = 66.1; // UMBmark prumer: kompenzace staceni doprava
    cfg.right_wheel_diameter = 65.9; // UMBmark prumer: kompenzace staceni doprava
    cfg.roztec_kol = 155.0; // v mm
    cfg.konstanta_radius_vnejsi_kolo = 0.96f; // Korekční faktor pro vnější kolo při zatáčení
    cfg.konstanta_radius_vnitrni_kolo = 0.96f; // Korekční faktor pro vnitřní kolo při zatáčení
    cfg.korekce_nedotacivosti_left = 0.958f; // Kalibrace plna baterie z 10x 90° (900/895 * 0.953)
    cfg.korekce_nedotacivosti_right = 0.948f; // Kalibrace z 10x 90° (900/902 * 0.950)
    cfg.Button1 = 27; // RIGHT
    cfg.Button2 = 14; // LEFT
    cfg.motor_id_left = 4;
    cfg.motor_id_right = 1;
    cfg.motor_max_power_pct = 100;
    cfg.motor_polarity_switch_left = true;
    cfg.motor_polarity_switch_right = false;
    cfg.motor_enable_failsafe = false;
    cfg.motor_wheel_diameter = 66.0;
    cfg.motor_max_ticks_per_second = 5200; // vyzkousite tak ze spustite funkci max_rychlost() a podle toho nastavite
    cfg.motor_max_acceleration = 50000;
    cfg.stupid_servo_min = -1.65f;
    cfg.stupid_servo_max = 1.65f;
    cfg.pocet_chytrych_serv = 2;
    cfg.enable_wifi_log = false;
    cfg.enable_wifi_control_wasd = false;
    cfg.enable_wifi_terminal = false;
    cfg.wifi_ssid = "robot1234";
    cfg.wifi_password = "1234robot";
    
    
    rkSetup(cfg);
    printf("Robotka started!\n");

    Serial.begin(115200);
    gyroInit();
   

    //rkCheckBattery(); 
    
    rkLedAll(false); // Vypneme LED na startu, menu se o ně postará


    // Nastavení Chytrých serv
    // Zpřísněný limit pro soft move (150 centistupňů = 1.5°) pro spolehlivější detekci překážky/náběru
    rkSmartServoInit(0, 0, 240, 150, 3);
    rkSmartServoInit(1, 0, 240);

    // Zvednuti ramene a presun na stred
    Rameno.Up();
    delay(2000); // Pockame, az vyjede nahoru
    Rameno.Center();
    delay(2000);

    Rameno.fSetDefaultSmartServosSpeed(200);

    
    //rkWaitForStart(); 
}
void loop() {



    // switch (eCurrentState) {
        
    //     // ========================================================
    //     // ROADSIDE MENU 1: Výběr Týmu (Barvy)
    //     // ========================================================

    /*
    switch (eCurrentState) {
        
        // ========================================================
        // ROADSIDE MENU 1: Výběr Týmu (Barvy)
        // ========================================================

        case MenuState::ROADSIDE_SELECT_TEAM:
            // Signalizace podle vybraného týmu (kompenzace za nefunkční modrou LED)
            rkLedAll(false);
            if (eSelectedTeam == TeamColor::Blue) rkLedGreen(true); // Zelená jako náhrada za modrou
            else if (eSelectedTeam == TeamColor::Red) rkLedRed(true);

            if (rkButton1(true)) { eSelectedTeam = TeamColor::Blue; printf("Tym: MODRA\n"); }
            if (rkButton2(true)) { eSelectedTeam = TeamColor::Red; printf("Tym: CERVENA\n"); }
            
            // Potvrzení MENU
            if (rkButtonOn(true)) {
                eCurrentState = MenuState::ROADSIDE_SELECT_LAYOUT;
                printf("Presun do: ROADSIDE MENU 2 (Vyber rozlozeni)\n");
            }
            break;

        // ========================================================
        // ROADSIDE MENU 2: Výběr Rozložení Baterií
        // ========================================================

        case MenuState::ROADSIDE_SELECT_LAYOUT:
            // Signalizace rozložení (0-3) pomocí barev (kompenzace za nefunkční modrou LED)
            rkLedAll(false);
            if (iSelectedLayout == 0) rkLedRed(true);
            else if (iSelectedLayout == 1) rkLedYellow(true);
            else if (iSelectedLayout == 2) rkLedGreen(true);
            else if (iSelectedLayout == 3) { rkLedRed(true); rkLedGreen(true); } // Červená + Zelená místo modré

            // -- Výběr hodnot
            if (rkButton1(true)) { 
                iSelectedLayout = (iSelectedLayout + 1) % 4; 
                printf("Rozlozeni (NEXT): %d\n", iSelectedLayout); 
            }
            if (rkButton2(true)) { 
                iSelectedLayout = (iSelectedLayout - 1 < 0) ? 3 : iSelectedLayout - 1; 
                printf("Rozlozeni (BACK): %d\n", iSelectedLayout); 
            }

            // Potvrzení MENU
            if (rkButtonOn(true)) {
                eCurrentState = MenuState::ROADSIDE_WAIT_START;
                printf("Presun do: ROADSIDE MENU 3 (Cekam na start)\n");
            }
            // Návrat do predchozího MENU
            if (rkButtonOff(true)) {
                eCurrentState = MenuState::ROADSIDE_SELECT_TEAM;
                printf("Zpet na: Vyber tymu\n");
            }
            break;

        // ========================================================
        // ROADSIDE MENU 3: START (ON)
        // ========================================================

        case MenuState::ROADSIDE_WAIT_START:
            // Blikání LED (kromě modré) na znamení připravenosti ke startu
            {
                bool val = (millis() % 500 < 250);
                rkLedRed(val);
                rkLedYellow(val);
                rkLedGreen(val);
            }

            // Potvrzení MENU ---> Spuštění Roadside
            if (rkButtonOn(true)) {
                rkLedAll(false); // Vypnutí LED
                RoadsideGame.fInitGame(iSelectedLayout, eSelectedTeam);
                bRoadsideGameStarted = true;
                eCurrentState = MenuState::GAME_RUNNING;
                GameTimer.fStart(); // Spuštění časovače
                printf("=== ROADSIDE ZAPAS ODSTARTOVAN! ===\n");
            }
            // Návrat do predchozího MENU
            if (rkButtonOff(true)) {
                eCurrentState = MenuState::ROADSIDE_SELECT_LAYOUT;
                printf("Zpet na: Vyber rozlozeni\n");
            }
            break;

        // ========================================================
        // FÁZE 2: HLAVNÍ JÍZDNÍ SMYČKA (Po odstartování)
        // ========================================================

        case MenuState::GAME_RUNNING:
            printf("=== START SOUTĚŽNÍ JÍZDY ===\n");
            
            // Úvodní blikání LED (červená, žlutá, zelená) - 5x
            for (int i = 0; i < 5; i++) {
                rkLedRed(true); rkLedYellow(true); rkLedGreen(true);
                delay(100);
                rkLedAll(false);
                delay(100);
            }

            // 1. FÁZE: Sebrání baterie (rychlost 40) -> Zelená LED
            rkLedGreen(true);
            RoadsideGame.fTakeClosestBattery(rCurrentRobotX, 40.0f);
            rkLedGreen(false);

            delay(1000);

            // 2. FÁZE: Odevzdání baterie do docku (rychlost 40) -> Červená LED
            rkLedRed(true);
            RoadsideGame.fFillClosestDock(rCurrentRobotX, 40.0f);
            rkLedRed(false);

            // Vítězný světelný efekt (střídavé blikání LED)
            for (int i = 0; i < 6; i++) {
                rkLedGreen(true); delay(100); rkLedGreen(false);
                rkLedYellow(true); delay(100); rkLedYellow(false);
                rkLedRed(true); delay(100); rkLedRed(false);
            }
            rkLedAll(false);
            printf("=== KONEC SOUTĚŽNÍ JÍZDY ===\n");

            // Návrat na začátek menu po dojetí
            eCurrentState = MenuState::ROADSIDE_SELECT_TEAM;
            bRoadsideGameStarted = false;
            break;
    }
    */

    // Tlačítko ON: Kompletní jízda 118 cm dopředu, nabrání baterky vpravo, odvezení a položení vlevo
    if (rkButtonOn(true)) {
        printf("=== START SEKVENČNÍ JÍZDY (VPRAVO -> VLEVO) ===\n");
        
        // Indikace LED a čekání 1 sekundu po stisku
        rkLedRed(true); rkLedYellow(true); rkLedGreen(true);
        delay(1000);
        rkLedAll(false);

        // 1. Jízda 118 cm dopředu -> Zelená LED
        rkLedGreen(true);
        move_acc_avoid(1180.0f, 40, []() { return false; }, 8000);
        rkLedGreen(false);

        // 2. Ruka doprava -> Žlutá LED
        rkLedYellow(true);
        Rameno.Side(Rameno.iRight);
        delay(1000);

        // 3. Ruka dolu
        Rameno.Down();
        delay(1000);

        // 4. Magnet ON (uchopit baterku)
        Rameno.Magnet(true);
        delay(1000);

        // 5. Ruka nahoru (výšková pozice po resetu)
        Rameno.Up();
        delay(1000);

        // 6. Ruka na střed (125 stupňů)
        Rameno.Center();
        delay(1000);
        rkLedYellow(false);

        // 7. Popojet dopředu o 35 cm -> Zelená LED
        rkLedGreen(true);
        move_acc_avoid(350.0f, 40, []() { return false; }, 4000);
        rkLedGreen(false);

        // 7b. Otočení podvozku o 90 stupňů doprava před vykládáním
        TurnOnSpotRight_acc(90, 40);
        delay(500);

        // 8. Vyložení baterky před sebe (ruka zůstává na středu) -> Žlutá LED
        rkLedYellow(true);
        Rameno.DownUnload();
        delay(1000);

        // 9. Magnet OFF (pustit baterku)
        Rameno.Magnet(false);
        delay(500); // počkáme půl sekundy dle zadání

        // 10. Ruka nahoru
        Rameno.Up();
        delay(1000);

        // 11. Ruka na střed
        Rameno.Center();
        delay(1000);
        rkLedYellow(false);

        // 11b. Otočení podvozku o 90 stupňů zpět doleva po vyložení
        TurnOnSpotLeft_acc(90, 40);
        delay(500);

        // 12. Popojet zpátky o 35 cm -> Červená LED
        rkLedRed(true);
        move_acc_avoid(-350.0f, 40, []() { return false; }, 4000);
        rkLedRed(false);

        // Indikace úspěšného konce
        for (int i = 0; i < 5; i++) {
            rkLedRed(true); rkLedYellow(true); rkLedGreen(true);
            delay(150);
            rkLedAll(false);
            delay(150);
        }
        printf("=== KONEC SEKVENČNÍ JÍZDY (VPRAVO -> VLEVO) ===\n");
    }

    // Tlačítko OFF: Kompletní jízda 118 cm dopředu, nabrání baterky vlevo, odvezení a položení vpravo
    if (rkButtonOff(true)) {
        printf("=== START SEKVENČNÍ JÍZDY (VLEVO -> VPRAVO) ===\n");
        
        // Indikace LED a čekání 1 sekundu po stisku
        rkLedRed(true); rkLedYellow(true); rkLedGreen(true);
        delay(1000);
        rkLedAll(false);

        // 1. Jízda 118 cm dopředu -> Zelená LED
        rkLedGreen(true);
        move_acc_avoid(1180.0f, 40, []() { return false; }, 8000);
        rkLedGreen(false);

        // 2. Ruka doleva -> Žlutá LED
        rkLedYellow(true);
        Rameno.Side(Rameno.iLeft);
        delay(1000);

        // 3. Ruka dolu
        Rameno.Down();
        delay(1000);

        // 4. Magnet ON (uchopit baterku)
        Rameno.Magnet(true);
        delay(1000);

        // 5. Ruka nahoru (výšková pozice po resetu)
        Rameno.Up();
        delay(1000);

        // 6. Ruka na střed (125 stupňů)
        Rameno.Center();
        delay(1000);
        rkLedYellow(false);

        // 7. Popojet dopředu o 35 cm -> Zelená LED
        rkLedGreen(true);
        move_acc_avoid(350.0f, 40, []() { return false; }, 4000);
        rkLedGreen(false);

        // 7b. Otočení podvozku o 90 stupňů doprava před vykládáním
        TurnOnSpotRight_acc(90, 40);
        delay(500);

        // 8. Vyložení baterky před sebe (ruka zůstává na středu) -> Žlutá LED
        rkLedYellow(true);
        Rameno.DownUnload();
        delay(1000);

        // 9. Magnet OFF (pustit baterku)
        Rameno.Magnet(false);
        delay(500); // počkáme půl sekundy dle zadání

        // 10. Ruka nahoru
        Rameno.Up();
        delay(1000);

        // 11. Ruka na střed
        Rameno.Center();
        delay(1000);
        rkLedYellow(false);

        // 11b. Otočení podvozku o 90 stupňů zpět doleva po vyložení
        TurnOnSpotLeft_acc(90, 40);
        delay(500);

        // 12. Popojet zpátky o 35 cm -> Červená LED
        rkLedRed(true);
        move_acc_avoid(-350.0f, 40, []() { return false; }, 4000);
        rkLedRed(false);

        // Indikace úspěšného konce
        for (int i = 0; i < 5; i++) {
            rkLedRed(true); rkLedYellow(true); rkLedGreen(true);
            delay(150);
            rkLedAll(false);
            delay(150);
        }
        printf("=== KONEC SEKVENČNÍ JÍZDY (VLEVO -> VPRAVO) ===\n");
    }
// ======= TESTOVACÍ ČÁSTI ==========

    
    // if (rkButtonIsPressed(BTN_LEFT) && rkButtonIsPressed(BTN_RIGHT))    {  }
    // if (rkButtonIsPressed(BTN_UP) && rkButtonIsPressed(BTN_DOWN))       {  }
    // if (rkButtonIsPressed(BTN_ON))      { }
    // if (rkButtonIsPressed(BTN_OFF))     { 
        
    //     // SCÉNÁŘ PRO MANIPULÁTOR 1 (ID 0)
        
    //          // 1. Otevři chapadlo
    //     delay(1500);
        
    //           // 2. Dojeď pro předmět před sebe                       | Cíl: X=180, Y=50, Základna = 0° (vpřed)
    //     delay(10000);
        
    //          // 3. Zavři chapadlo (chytni předmět)
    //     delay(1500);    
        
    //         // 4. Zvedni ho do výšky a otoč se s ním doprava        | Cíl: X=180, Y=50, Základna = -90° (vpravo)
    //     delay(10000);
        
    //           // 5. Otevři chapadlo (pusť předmět)
    //     delay(1500);

    //     printf("\n");
        
    // }

    // Tlačítko UP: Rameno doprava (iRight) a dolů (nabírání baterky vpravo)
    if (rkButtonIsPressed(BTN_UP)) {
        printf("=== UP: DOPRAVA A DOLŮ ===\n");
        rkLedYellow(true);
        Rameno.Side(Rameno.iRight);
        delay(1000);
        Rameno.Down();
        delay(1000);
        rkLedYellow(false);
    }

    // Tlačítko DOWN: Kalibrace gyroskopu a vypisování dat
    if (rkButtonIsPressed(BTN_DOWN)) {
        printf("=== DOWN: Inicializace gyroskopu... ===\n");
        gyroInit();
        
        printf("=============================================\n");
        printf("=== START KALIBRACE GYROSKOPU (DOWN) ===\n");
        printf("=== UDRŽUJTE ROBOTA V KLIDU! ===\n");
        printf("=============================================\n");
        
        // Signalizace kalibrace žlutou LED
        rkLedYellow(true);
        gyroRequestCalibration();
        delay(1500);
        rkLedYellow(false);
        
        printf("=== Kalibrace dokončena! Vypisuji data z gyroskopu. ===\n");
        printf("=== (Pro ukončení stiskněte libovolné jiné tlačítko) ===\n");
        
        while (!rkButtonIsPressed(BTN_UP) && !rkButtonIsPressed(BTN_LEFT) && !rkButtonIsPressed(BTN_RIGHT) && !rkButtonIsPressed(BTN_ON) && !rkButtonIsPressed(BTN_OFF)) {
            auto &gs = getGyroState();
            printf("MPU6050 -> Task Read OK: %s | Raw Gyro Z: %.6f rad/s | Offset: %.6f rad/s | Angle Z: %.2f deg | Acc Z: %.3f m/s^2\n", 
                   gs.lastReadOk ? "YES" : "NO", gs.lastRawGyroZ, gs.rawOffsetZ, gyroGetAngleZ(), gs.lastRawAccZ);
            rkLedRed(true);
            delay(100);
            rkLedRed(false);
            delay(100);
        }
        printf("=== Vypisování gyro dat ukončeno. ===\n");
    }
    

    // TEST 10x 90 stupnu VLEVO
    if (rkButtonLeft(true)) {
        printf("Test 10x 90 stupnu VLEVO...\n");
        for (int i = 0; i < 10; i++) {
            TurnOnSpotLeft_acc(90, 40);
            delay(300);
        }
        printf("Test VLEVO dokoncen!\n");
    }
    // TEST 10x 90 stupnu VPRAVO
    if (rkButtonRight(true)) { 
        printf("Test 10x 90 stupnu VPRAVO...\n");
        for (int i = 0; i < 10; i++) {
            TurnOnSpotRight_acc(90, 40);
            delay(300);
        }
        printf("Test VPRAVO dokoncen!\n");
    }
    
    // KALIBRACE UMBMARK - Ctverec Vpravo (Po smeru hodinovych rucicek)
    // if (rkButton1(true)) { 
    //     printf("Startuji kalibracni ctverec VPRAVO (1x)...\n");
    //     for (int i = 0; i < 1; i++) {
    //         for (int j = 0; j < 4; j++) {
    //             move_acc_avoid(1000.0f, 40, []() { return false; }, 8000);
    //             delay(300);
    //             TurnOnSpotRight_acc(90, 40);
    //             delay(300);
    //         }
    //     }
    //     printf("Kalibrace VPRAVO dokoncena!\n");
    // }
    
    // KALIBRACE UMBMARK - Ctverec Vlevo (Proti smeru hodinovych rucicek)
    // if (rkButton2(true)) { 
    //     printf("Startuji kalibracni ctverec VLEVO (1x)...\n");
    //     for (int i = 0; i < 1; i++) {
    //         for (int j = 0; j < 4; j++) {
    //             move_acc_avoid(1000.0f, 40, []() { return false; }, 8000);
    //             delay(300);
    //             TurnOnSpotLeft_acc(90, 40);
    //             delay(300);
    //         }
    //     }
    //     printf("Kalibrace VLEVO dokoncena!\n");
    // }



    delay(10);
}

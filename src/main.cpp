#include "robotka.h"

#include <Arduino.h>
#include <string>

#include "Roadside.h"
#include "Movement.h"
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

int input_speed = 100;
float speed_forward_corr = 1;
float speed_backward_corr = 1;
byte inputBT = 0;   

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
    if (!SerialBT.begin("Antenka")) {
        printf("Bluetooth Fail!");
    } 
    else { 
        printf("Bluetooth started.\n");
        rkLedAll(true);
        delay(100);
    }

    Serial.begin(115200);
   

    //rkCheckBattery(); 
    
    rkLedAll(false); // Vypneme LED na startu, menu se o ně postará


    // Nastavení Chytrých serv
    rkSmartServoInit(0, 0, 240, 500, 3);
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

    //     case MenuState::ROADSIDE_SELECT_TEAM:
    //         // Signalizace podle vybraného týmu
    //         rkLedAll(false);
    //         if (eSelectedTeam == TeamColor::Blue) rkLedBlue(true);
    //         else if (eSelectedTeam == TeamColor::Red) rkLedRed(true);

    //         if (rkButton1(true)) { eSelectedTeam = TeamColor::Blue; printf("Tym: MODRA\n"); }
    //         if (rkButton2(true)) { eSelectedTeam = TeamColor::Red; printf("Tym: CERVENA\n"); }
            
    //         // Potvrzení MENU
    //         if (rkButtonOn(true)) {
    //             eCurrentState = MenuState::ROADSIDE_SELECT_LAYOUT;
    //             printf("Presun do: ROADSIDE MENU 2 (Vyber rozlozeni)\n");
    //         }
    //         break;

    //     // ========================================================
    //     // ROADSIDE MENU 2: Výběr Rozložení Baterií
    //     // ========================================================

    //     case MenuState::ROADSIDE_SELECT_LAYOUT:
    //         // Signalizace rozložení (0-3) pomocí barev
    //         rkLedAll(false);
    //         if (iSelectedLayout == 0) rkLedRed(true);
    //         else if (iSelectedLayout == 1) rkLedYellow(true);
    //         else if (iSelectedLayout == 2) rkLedGreen(true);
    //         else if (iSelectedLayout == 3) rkLedBlue(true);

    //         // -- Výběr hodnot
    //         if (rkButton1(true)) { 
    //             iSelectedLayout = (iSelectedLayout + 1) % 4; 
    //             printf("Rozlozeni (NEXT): %d\n", iSelectedLayout); 
    //         }
    //         if (rkButton2(true)) { 
    //             iSelectedLayout = (iSelectedLayout - 1 < 0) ? 3 : iSelectedLayout - 1; 
    //             printf("Rozlozeni (BACK): %d\n", iSelectedLayout); 
    //         }

    //         // Potvrzení MENU
    //         if (rkButtonOn(true)) {
    //             eCurrentState = MenuState::ROADSIDE_WAIT_START;
    //             printf("Presun do: ROADSIDE MENU 3 (Cekam na start)\n");
    //         }
    //         // Návrat do predchozího MENU
    //         if (rkButtonOff(true)) {
    //             eCurrentState = MenuState::ROADSIDE_SELECT_TEAM;
    //             printf("Zpet na: Vyber tymu\n");
    //         }
    //         break;

    //     // ========================================================
    //     // ROADSIDE MENU 3: START (ON)
    //     // ========================================================

    //     case MenuState::ROADSIDE_WAIT_START:
    //         // Blikání všech LED na znamení připravenosti ke startu
    //         rkLedAll(millis() % 500 < 250);

    //         // Potvrzení MENU ---> Spuštění Roadside
    //         if (rkButtonOn(true)) {
    //             rkLedAll(false); // Vypnutí LED
    //             RoadsideGame.fInitGame(iSelectedLayout, eSelectedTeam);
    //             bRoadsideGameStarted = true;
    //             eCurrentState = MenuState::GAME_RUNNING;
    //             GameTimer.fStart(); // Spuštění vlákna pro odpočet času
    //             printf("=== ROADSIDE ZAPAS ODSTARTOVAN! ===\n");
    //         }
    //         // Návrat do predchozího MENU
    //         if (rkButtonOff(true)) {
    //             eCurrentState = MenuState::ROADSIDE_SELECT_LAYOUT;
    //             printf("Zpet na: Vyber rozlozeni\n");
    //         }
    //         break;

    //     // ========================================================
    //     // FÁZE 2: HLAVNÍ JÍZDNÍ SMYČKA (Po odstartování)
    //     // ========================================================

    //     case MenuState::GAME_RUNNING:
            
    //         // Zjistíme, jestli nám nedochází čas (limit 300s, tolerance např. 45s pro návrat)
    //         if (GameTimer.bIsTimeRunningOut(300, 45)) {
    //             printf("[CAS!] Zbyva malo casu (ubehlo %d s)! Ukoncuji sber a vracim se na start...\n", GameTimer.iGetElapsedSeconds());
    //             // Následoval by kód pro odjezd domů
    //             }

    //             // Tady už běží samotná odometrie a logika soutěže ROADSIDE
                
    //             // Zkusi sebrat baterii s vyhýbáním překážkám
    //             RoadsideGame.fTakeClosestBattery(rCurrentRobotX);

    //         break;
    // }
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

    if (SerialBT.available() > 0) {
        inputBT = SerialBT.read();
        SerialBT.print("Prislo: "); SerialBT.println(inputBT );
        if ( inputBT == 117) {
            input_speed += 5;
            SerialBT.print("speed: "); SerialBT.println(input_speed);
            Serial.print("speed: "); Serial.println(input_speed);
        }
            
        if ( inputBT == 100) {
            input_speed -= 5;   
            SerialBT.print("speed: "); SerialBT.println(input_speed);
            Serial.print("speed: "); Serial.println(input_speed);
        }

        if ( inputBT == 110) { // pismeno n = "nahoru"
            rameno_pos -= 5;
            rkSmartServoSoftMove(0, rameno_pos, rDefSpeed ); 
            SerialBT.print(" RP "); SerialBT.print(rameno_pos); Serial.print(" RP "); Serial.print(rameno_pos);
        }

        if ( inputBT == 113) { // pismeno q = "dolu" (d snizuje rychlost)
            rameno_pos += 5;
            rkSmartServoSoftMove(0, rameno_pos, rDefSpeed ); 
            SerialBT.print(" RP "); SerialBT.print(rameno_pos); Serial.print(" RP "); Serial.print(rameno_pos);
        }

    }



    // testovací jízda vpřed a vzad 
    if ( (rkButtonIsPressed(BTN_UP)) || ( inputBT == 118) )    
        { delay(1000); move_acc_avoid(2000.0f, input_speed, []() { return false; }, 8000); }
    if ( (rkButtonIsPressed(BTN_DOWN)) || ( inputBT == 122) )   
        { delay(1000); move_acc_avoid(-2000.0f, input_speed, []() { return false; }, 8000); }
    

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

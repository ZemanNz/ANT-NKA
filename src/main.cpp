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
    
    
    Serial.begin(115200);
    delay(10);
    Serial.println("=== DIAGNOSTICKÝ START SETUPU ===");
    uint32_t t_start = millis();

    rkSetup(cfg);
    Serial.printf("[SETUP] rkSetup trval: %lu ms\n", (unsigned long)(millis() - t_start));
    
    uint32_t t_gyro = millis();
    gyroInit();
    Serial.printf("[SETUP] gyroInit trval: %lu ms\n", (unsigned long)(millis() - t_gyro));

    //rkCheckBattery(); 
    
    rkLedAll(false); // Vypneme LED na startu, menu se o ně postará

    // Nastavení Chytrých serv
    // Zpřísněný limit pro soft move (150 centistupňů = 1.5°) pro spolehlivější detekci překážky/náběru
    uint32_t t_servos_init = millis();
    rkSmartServoInit(0, 0, 240, 150, 3);
    rkSmartServoInit(1, 0, 240);
    Serial.printf("[SETUP] rkSmartServoInit trval: %lu ms\n", (unsigned long)(millis() - t_servos_init));

    // Zvednuti ramene a presun na stred (wait_for_servo() už v sobě má dostatečné čekání a 500ms delay)
    uint32_t t_arm_up = millis();
    Rameno.Up();
    Serial.printf("[SETUP] Rameno.Up() trval: %lu ms\n", (unsigned long)(millis() - t_arm_up));
    
    uint32_t t_arm_center = millis();
    Rameno.Center();
    Serial.printf("[SETUP] Rameno.Center() trval: %lu ms\n", (unsigned long)(millis() - t_arm_center));

    Rameno.fSetDefaultSmartServosSpeed(60);

    // Indikace dokončení inicializace a připravenosti robota (lze stisknout ON)
    rkLedYellow(true);
    Serial.printf("[SETUP] Celkový čas setupu: %lu ms\n", (unsigned long)(millis() - t_start));
    Serial.println("=================================");
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

    // Tlačítko ON: Kompletní soutěžní sekvence (3 baterky)
    if (rkButtonOn(true)) {
        printf("=== START SOUTĚŽNÍ SEKVENČNÍ JÍZDY ===\n");
        
        // Indikace LED a čekání 1 sekundu po stisku
        rkLedRed(true); rkLedYellow(true); rkLedGreen(true);
        delay(1000);
        rkLedAll(false);

        // Vynulování absolutního gyroskopu na začátku celé jízdy
        gyroResetZ();
        delay(50);

        // ================== 1. BATERKA ==================
        // 1. Jízda 118 cm dopředu k 1. baterce -> Zelená LED
        printf("Jízda 118 cm k 1. baterce...\n");
        rkLedGreen(true);
        move_straight_gyro(1180.0f, 40, 8000, 0.0f); // Sledujeme směr 0.0
        rkLedGreen(false);

        // 2. Uchopení 1. baterky otočením doleva -> Žlutá LED
        printf("Otočení doleva a uchopení 1. baterky...\n");
        rkLedYellow(true);
        turn_gyro(90.0f, 40);
        align_gyro(90.0f, 30);
        Rameno.Center();
        delay(1000);
        Rameno.Down();
        delay(1000);
        Rameno.Magnet(true);
        delay(1000);
        Rameno.Up();
        delay(1000);
        printf("Otočení zpět na směr 0.0...\n");
        turn_gyro(0.0f, 40);
        align_gyro(0.0f, 30);
        rkLedYellow(false);

        // 3. Popojet dopředu o 35 cm k 1. vykládací zóně -> Zelená LED
        printf("Popojetí 35 cm k vykládací zóně...\n");
        rkLedGreen(true);
        move_straight_gyro(350.0f, 40, 4000, 0.0f); // Sledujeme směr 0.0
        rkLedGreen(false);

        // 4. Vyložení 1. baterky vlevo (bez otáčení robota, šetrnější výška 95°) -> Žlutá LED
        printf("Vyložení 1. baterky vlevo...\n");
        rkLedYellow(true);
        align_gyro(0.0f, 30); // Dorovnání před pohybem servo 1 (Side)
        Rameno.Side(Rameno.iLeft);
        delay(1000);
        Rameno.DownUnload(); // Výška 95 stupňů
        delay(1000);
        Rameno.Magnet(false);
        delay(500);
        Rameno.Up();
        delay(1000);
        align_gyro(0.0f, 30); // Dorovnání před pohybem servo 1 (Center)
        Rameno.Center();
        delay(1000);
        rkLedYellow(false);

        // ================== 2. BATERKA ==================
        // 5. Zacouvat zpět o 40 cm (153 cm -> 113 cm, tj. 118 cm s přejetím o 5 cm) -> Červená LED
        printf("Zacouvání 40 cm zpět na úroveň 113 cm (sběr s přejezdem o 5 cm)...\n");
        rkLedRed(true);
        move_straight_gyro(-400.0f, 40, 4000, 0.0f); // Sledujeme směr 0.0
        rkLedRed(false);

        // 6. Uchopení 2. baterky otočením doleva -> Žlutá LED
        printf("Otočení doleva a uchopení 2. baterky...\n");
        rkLedYellow(true);
        turn_gyro(90.0f, 40);
        align_gyro(90.0f, 30);
        Rameno.Center();
        delay(1000);
        Rameno.Down();
        delay(1000);
        Rameno.Magnet(true);
        delay(1000);
        Rameno.Up();
        delay(1000);
        printf("Otočení zpět na směr 0.0...\n");
        turn_gyro(0.0f, 40);
        align_gyro(0.0f, 30);
        rkLedYellow(false);

        // 7. Vyložení 2. baterky vlevo (otočení ramene o 180° bez pohybu podvozku) -> Žlutá LED
        printf("Vyložení 2. baterky vlevo (180° otočení ramene)...\n");
        rkLedYellow(true);
        align_gyro(0.0f, 30); // Dorovnání před pohybem servo 1 (Side)
        Rameno.Side(Rameno.iLeft); // 180 stupňů od iRight
        delay(1000);
        Rameno.DownUnload(); // Výška 95 stupňů
        delay(1000);
        Rameno.Magnet(false);
        delay(500);
        Rameno.Up();
        delay(1000);
        align_gyro(0.0f, 30); // Dorovnání před pohybem servo 1 (Center)
        Rameno.Center();
        delay(1000);
        rkLedYellow(false);

        // ================== 3. BATERKA ==================
        // 8. Popojet jenom 5 cm dopředu pro 3. baterku (113 cm -> 118 cm) -> Zelená LED
        printf("Popojetí 5 cm dopředu k 3. baterce...\n");
        rkLedGreen(true);
        move_straight_gyro(50.0f, 40, 1000, 0.0f); // Sledujeme směr 0.0
        rkLedGreen(false);

        // 9. Uchopení 3. baterky otočením doleva -> Žlutá LED
        printf("Otočení doleva a uchopení 3. baterky...\n");
        rkLedYellow(true);
        turn_gyro(90.0f, 40);
        align_gyro(90.0f, 30);
        Rameno.Center();
        delay(1000);
        Rameno.Down();
        delay(1000);
        Rameno.Magnet(true);
        delay(1000);
        Rameno.Up();
        delay(1000);
        printf("Otočení zpět na směr 0.0...\n");
        turn_gyro(0.0f, 40);
        align_gyro(0.0f, 30);
        rkLedYellow(false);

        // 10. Zacouvat zpět o 16 cm k vyložení (118 cm -> 102 cm) -> Červená LED
        printf("Zacouvání 16 cm k vyložení 3. baterky...\n");
        rkLedRed(true);
        move_straight_gyro(-160.0f, 40, 2000, 0.0f); // Sledujeme směr 0.0
        rkLedRed(false);

        // 11. Vyložení 3. baterky na bok (vlevo) -> Žlutá LED
        printf("Vyložení 3. baterky na bok (vlevo)...\n");
        rkLedYellow(true);
        align_gyro(0.0f, 30); // Dorovnání před pohybem servo 1 (Side)
        Rameno.Side(Rameno.iLeft);
        delay(1000);
        Rameno.DownUnload(); // Výška 95 stupňů
        delay(1000);
        Rameno.Magnet(false);
        delay(500);
        Rameno.Up();
        delay(1000);
        align_gyro(0.0f, 30); // Dorovnání před pohybem servo 1 (Center)
        Rameno.Center();
        delay(1000);
        rkLedYellow(false);

        // ================== 4. BATERKA ==================
        // 12. Popojet 24 cm dopředu pro 4. baterku (102 cm -> 126 cm) -> Zelená LED
        printf("Popojetí 24 cm dopředu k 4. baterce...\n");
        rkLedGreen(true);
        move_straight_gyro(240.0f, 40, 3000, 0.0f); // Sledujeme směr 0.0
        rkLedGreen(false);

        // 13. Uchopení 4. baterky otočením doleva -> Žlutá LED
        printf("Otočení doleva a uchopení 4. baterky...\n");
        rkLedYellow(true);
        turn_gyro(90.0f, 40);
        align_gyro(90.0f, 30);
        Rameno.Center();
        delay(1000);
        Rameno.Down();
        delay(1000);
        Rameno.Magnet(true);
        delay(1000);
        Rameno.Up();
        delay(1000);
        printf("Otočení zpět na směr 0.0...\n");
        turn_gyro(0.0f, 40);
        align_gyro(0.0f, 30);
        rkLedYellow(false);

        // 14. Zacouvat zpět o 75 cm k vyložení 4. baterky (126 cm -> 51 cm) -> Červená LED
        printf("Zacouvání 75 cm zpět k vyložení...\n");
        rkLedRed(true);
        move_straight_gyro(-750.0f, 40, 5000, 0.0f); // Sledujeme směr 0.0
        rkLedRed(false);

        // 15. Vyložení 4. baterky na stejnou stranu jako 1. baterku (vlevo/doprava podle servo pozice iLeft) -> Žlutá LED
        printf("Vyložení 4. baterky vlevo...\n");
        rkLedYellow(true);
        align_gyro(0.0f, 30); // Dorovnání před pohybem servo 1 (Side)
        Rameno.Side(Rameno.iLeft);
        delay(1000);
        Rameno.DownUnload(); // Výška 95 stupňů
        delay(1000);
        Rameno.Magnet(false);
        delay(500);
        Rameno.Up();
        delay(1000);
        align_gyro(0.0f, 30); // Dorovnání před pohybem servo 1 (Center)
        Rameno.Center();
        delay(1000);
        rkLedYellow(false);

        // Indikace úspěšného konce
        for (int i = 0; i < 5; i++) {
            rkLedRed(true); rkLedYellow(true); rkLedGreen(true);
            delay(150);
            rkLedAll(false);
            delay(150);
        }
        printf("=== KONEC SEKVENČNÍ JÍZDY ===\n");
    }

    // Tlačítko OFF: Samostatné vyrovnání robota na směr 0.0 stupňů (pro testy)
    if (rkButtonOff(true)) {
        printf("=== START TESTOVACÍHO OTOČENÍ O 90 STUPŇŮ (OFF) ===\n");
        
        // Indikace LED a čekání 1 sekundu po stisku
        rkLedRed(true); rkLedYellow(true); rkLedGreen(true);
        delay(1000);
        rkLedAll(false);

        // Reset gyroskopu před začátkem otočení
        gyroResetZ();
        delay(50);

        // Otočení o 90 stupňů vlevo a následné přesné dorovnání
        turn_gyro(90.0f, 40);
        align_gyro(90.0f, 30);

        // Indikace úspěšného konce
        for (int i = 0; i < 5; i++) {
            rkLedRed(true); rkLedYellow(true); rkLedGreen(true);
            delay(150);
            rkLedAll(false);
            delay(150);
        }
        printf("=== KONEC OTOČENÍ ===\n");
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

    // Tlačítko DOWN: Reset úhlu gyroskopu a vypisování dat
    if (rkButtonIsPressed(BTN_DOWN)) {
        printf("=== DOWN: Inicializace a reset gyroskopu... ===\n");
        gyroInit();
        gyroResetZ();
        
        printf("=== Gyro resetováno! Vypisuji data z gyroskopu. ===\n");
        printf("=== (Pro ukončení stiskněte libovolné jiné tlačítko) ===\n");
        
        while (!rkButtonIsPressed(BTN_UP) && !rkButtonIsPressed(BTN_LEFT) && !rkButtonIsPressed(BTN_RIGHT) && !rkButtonIsPressed(BTN_ON) && !rkButtonIsPressed(BTN_OFF)) {
            printf("Gyro Angle Z: %.2f\n", gyroGetAngleZ());
            delay(100);
        }
        printf("=== Vypisování gyro dat ukončeno. ===\n");
    }
    

    // Tlačítko LEFT: Otočení o 90 stupňů VLEVO (nízká rychlost)
    if (rkButtonLeft(true)) {
        printf("Tlačítko LEFT: Otočení o 90 stupňů VLEVO...\n");
        turn_gyro(90, 25);
        printf("Otočení VLEVO dokončeno!\n");
    }
    // Tlačítko RIGHT: Otočení o 90 stupňů VPRAVO (nízká rychlost)
    if (rkButtonRight(true)) { 
        printf("Tlačítko RIGHT: Otočení o 90 stupňů VPRAVO...\n");
        turn_gyro(-90, 25);
        printf("Otočení VPRAVO dokončeno!\n");
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

#include "robotka.h"

#include <Arduino.h>
#include <string>

#include "GameDimensions.h"
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


float rCurrentRobotX = GameDimensions::POCATECNI_X_STRED_MM; // 330 mm (střed robota od zdi)
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

    // ================================================================
    // SOUTĚŽNÍ FUNKCE
    // ================================================================

    // Aktuální pozice středu robota na hřišti (osa X, od levé zdi)
    // RED startuje vpravo (X≈2670), BLUE vlevo (X≈330)
    // "Dopředu" pro RED = směr -X, pro BLUE = směr +X

    /**
     * Dojeď tak, aby rameno bylo nad cílovou X pozicí.
     * Automaticky počítá s offsetem ramena a směrem jízdy dle barvy týmu.
     */
    auto dojed_k = [](float& current_x, float target_arm_x, bool isRed, float spd = 35.0f) {
        float center_target;
        if (isRed) {
            // RED jede v -X: rameno je NAPRAVO od středu (vyšší X)
            center_target = target_arm_x - GameDimensions::RAMENO_OD_STREDU_MM;
        } else {
            // BLUE jede v +X: rameno je NALEVO od středu (nižší X)
            center_target = target_arm_x + GameDimensions::RAMENO_OD_STREDU_MM;
        }
        float distance = isRed ? (current_x - center_target) : (center_target - current_x);
        printf("[DOJED] Z X=%.0f -> cíl ramene X=%.0f (střed=%.0f), vzdálenost=%.0f mm\n",
               current_x, target_arm_x, center_target, distance);
        
        uint32_t timeout = (uint32_t)(std::abs(distance) / 0.3f) + 3000; // dynamický timeout
        MoveResult res = move_straight_gyro(distance, spd, timeout, 0.0f);
        
        // Aktualizace pozice
        if (isRed) { current_x -= res.traveled_mm; }
        else       { current_x += res.traveled_mm; }
        printf("[DOJED] Nová pozice středu: X=%.0f mm\n", current_x);
    };

    /**
     * Chyť baterku: otoč se k bateriím, seber ramenem, otoč zpět.
     * Končí s ramenem v pozici Up+Center (reset).
     */
    auto chyt_baterku = [](bool isRed) {
        float grab_angle = isRed ? -90.0f : 90.0f;
        printf("[GRAB] Otáčím se o %.0f° k bateriím...\n", grab_angle);
        
        turn_gyro(grab_angle, 40);
        align_gyro(grab_angle, 30);
        
        Rameno.Center();
        delay(800);
        Rameno.Down();
        delay(800);
        Rameno.Magnet(true);
        delay(600);
        Rameno.Up();
        delay(800);
        
        printf("[GRAB] Otáčím zpět na 0°...\n");
        turn_gyro(0.0f, 40);
        align_gyro(0.0f, 30);
        
        Rameno.Center();
        delay(300);
        printf("[GRAB] Baterka chycena!\n");
    };

    /**
     * Pusť baterku do docku: rameno na bok, dolů, pusť, zpět.
     * Končí s ramenem v pozici Up+Center (reset) a magnetem zapnutým (pro další baterku).
     */
    auto pust_baterku = [](bool isRed) {
        // RED: dock je vlevo od robota (iLeft), BLUE: dock je vpravo (iRight)
        // ...ale záleží na fyzickém uspořádání. Používáme logiku z Roadside.h:
        int drop_side = isRed ? Rameno.iLeft : Rameno.iRight;
        printf("[DROP] Vykládám baterku na stranu %d...\n", drop_side);
        
        align_gyro(0.0f, 30);
        Rameno.Side(drop_side);
        delay(800);
        Rameno.DownUnload();
        delay(800);
        Rameno.Magnet(false);
        delay(500);
        Rameno.Up();
        delay(800);
        
        align_gyro(0.0f, 30);
        Rameno.Center();
        delay(300);
        
        // Nachystání magnetu na další baterku
        Rameno.Magnet(true);
        delay(200);
        printf("[DROP] Baterka vložena do docku!\n");
    };

    // ================================================================
    // TLAČÍTKO ON: SOUTĚŽNÍ JÍZDA
    // ================================================================
    if (rkButtonOn(true)) {
        // === NASTAVENÍ ZÁPASU ===
        bool isRed = true;                // <<< ZDE ZMĚNIT BARVU TÝMU
        int layout_idx = 0;              // <<< ZDE ZMĚNIT KOMBINACI (0-3)
        float race_speed = 35.0f;        // Rychlost jízdy (%)
        
        // Počáteční pozice (RED = pravá strana, BLUE = levá)
        float pos_x;
        if (isRed) {
            pos_x = GameDimensions::FIELD_WIDTH_MM - GameDimensions::STARTZONE_CENTER_MM 
                    - GameDimensions::ZADEK_OD_STREDU_MM; // 2670 mm
        } else {
            pos_x = GameDimensions::POCATECNI_X_STRED_MM; // 330 mm
        }

        printf("=== SOUTĚŽNÍ JÍZDA ===\n");
        printf("Tým: %s | Kombinace: %d | Start X: %.0f mm\n", 
               isRed ? "ČERVENÝ" : "MODRÝ", layout_idx, pos_x);

        // Indikace + čekání
        rkLedRed(true); rkLedYellow(true); rkLedGreen(true);
        delay(1000);
        rkLedAll(false);

        // Reset gyroskopu
        gyroResetZ();
        delay(50);

        // === ROZLOŽENÍ DOCKŮ ===
        const TeamColor* layout = GameDimensions::DOCK_LAYOUTS[layout_idx];
        TeamColor myColor = isRed ? TeamColor::Red : TeamColor::Blue;

        // Najdi 4 naše docky a seřaď od nejvzdálenějšího
        int my_docks[4];
        int dock_count = 0;
        for (int i = 0; i < 8 && dock_count < 4; i++) {
            if (layout[i] == myColor) {
                my_docks[dock_count++] = i;
            }
        }
        // Seřazení: nejvzdálenější od startu jako první
        for (int i = 0; i < dock_count - 1; i++) {
            for (int j = i + 1; j < dock_count; j++) {
                float di = std::abs(GameDimensions::DOCK_X_POSITIONS[my_docks[i]] - pos_x);
                float dj = std::abs(GameDimensions::DOCK_X_POSITIONS[my_docks[j]] - pos_x);
                if (dj > di) { int tmp = my_docks[i]; my_docks[i] = my_docks[j]; my_docks[j] = tmp; }
            }
        }

        // === SLOUPCE BATERIÍ: každou z jiného sloupce, nejvzdálenější první ===
        float bat_cols_sorted[4] = {
            GameDimensions::BATTERY_COL1_X_MM, GameDimensions::BATTERY_COL2_X_MM,
            GameDimensions::BATTERY_COL3_X_MM, GameDimensions::BATTERY_COL4_X_MM
        };
        // Seřazení: nejvzdálenější od startu první
        for (int i = 0; i < 3; i++) {
            for (int j = i + 1; j < 4; j++) {
                float di = std::abs(bat_cols_sorted[i] - pos_x);
                float dj = std::abs(bat_cols_sorted[j] - pos_x);
                if (dj > di) { float tmp = bat_cols_sorted[i]; bat_cols_sorted[i] = bat_cols_sorted[j]; bat_cols_sorted[j] = tmp; }
            }
        }

        printf("Pořadí docků: ");
        for (int i = 0; i < dock_count; i++) printf("%d(%.0f) ", my_docks[i]+1, GameDimensions::DOCK_X_POSITIONS[my_docks[i]]);
        printf("\nPořadí baterií: ");
        for (int i = 0; i < 4; i++) printf("%.0f ", bat_cols_sorted[i]);
        printf("\n");

        // === HLAVNÍ SMYČKA: 4× seber baterku a vlož do docku ===
        int delivered = 0;
        for (int cycle = 0; cycle < 4 && cycle < dock_count; cycle++) {
            printf("\n======== CYKLUS %d/4 ========\n", cycle + 1);
            
            // LED indikace cyklu
            rkLedGreen(true);
            
            // 1. Dojeď k baterii
            float bat_x = bat_cols_sorted[cycle];
            printf("[%d] Jedu k baterii na X=%.0f...\n", cycle+1, bat_x);
            dojed_k(pos_x, bat_x, isRed, race_speed);
            delay(200);
            
            // 2. Chyť baterku
            rkLedYellow(true);
            chyt_baterku(isRed);
            rkLedGreen(false);
            delay(200);
            
            // 3. Dojeď k docku
            float dock_x = GameDimensions::DOCK_X_POSITIONS[my_docks[cycle]];
            printf("[%d] Jedu k docku %d na X=%.0f...\n", cycle+1, my_docks[cycle]+1, dock_x);
            rkLedRed(true);
            dojed_k(pos_x, dock_x, isRed, race_speed);
            delay(200);
            
            // 4. Pusť baterku
            pust_baterku(isRed);
            delivered++;
            rkLedAll(false);
            
            printf("[%d] Baterka %d doručena do docku %d! (%d/4)\n", 
                   cycle+1, cycle+1, my_docks[cycle]+1, delivered);
            delay(200);
        }

        // === NÁVRAT DOMŮ: Zacouvej ke zdi a naraz tlačítkama ===
        printf("\n=== NÁVRAT DOMŮ ===\n");
        rkLedRed(true); rkLedGreen(true);
        
        // Dorovnej směr
        align_gyro(0.0f, 30);
        
        // Zacouvej ke zdi (jedeme POZADU = záporná vzdálenost pro forward)
        // RED: zpět = +X směr = backward, BLUE: zpět = -X = backward
        float home_x = isRed 
            ? (GameDimensions::FIELD_WIDTH_MM - 50.0f)   // Pravá zeď
            : 50.0f;                                       // Levá zeď
        float back_distance = std::abs(pos_x - home_x) + 100.0f; // + 100mm pro jistotu nárazu
        printf("[HOME] Couvám %.0f mm ke zdi...\n", back_distance);
        move_straight_gyro(-back_distance, 25, 8000, 0.0f);  // Záporná = couvání
        
        // Stop
        rkMotorsSetSpeed(0, 0);
        delay(500);

        // === KONEC: Pípnutí a blikání ===
        printf("=== KONEC ZÁPASU === Doručeno: %d/4 baterií\n", delivered);
        for (int i = 0; i < 8; i++) {
            rkLedRed(i % 2 == 0); rkLedYellow(i % 2 == 1); rkLedGreen(i % 2 == 0);
            delay(200);
            rkLedAll(false);
            delay(100);
        }
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

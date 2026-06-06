#include "robotka.h"

#include <Arduino.h>
#include <string>

#include "GameDimensions.h"
#include "Roadside.h"
#include "Movement.h"
#include "movement_gyro.h"
#include "ContestTimer.h"
#include "Shoulder.h"
#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include "OpponentDetection.h"


// Nastavení Roadsidu
GameManager RoadsideGame;

// Nastavení Časovače
ContestTimer GameTimer; 

// Globální instance Ramene (propojí se přes extern všude tam, kde je Shoulder.h) 
Shoulder Rameno;

// Globální proměnná pro detekci soupeře (task na pozadí ji nastavuje)
volatile bool opponentDetected = false;

// Instance laserového ToF senzoru VL53L0X
Adafruit_VL53L0X loxLaser = Adafruit_VL53L0X();

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

    uint32_t t_laser = millis();
    Serial.println("[SETUP] Initializing Wire1 on pins 21 (SDA), 22 (SCL) for laser...");
    pinMode(21, PULLUP);
    pinMode(22, PULLUP);
    Wire1.begin(21, 22, 400000);
    Wire1.setTimeOut(1);
    
    // Inicializace laseru
    rk_laser_init("laser", Wire1, loxLaser, 23, 0x30);
    Serial.printf("[SETUP] Laser init trval: %lu ms\n", (unsigned long)(millis() - t_laser));

    // Inicializace asynchronní detekce soupeře
    opponentDetectionInit();

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
    // === NASTAVENÍ PRO ZÁPAS / TESTY (lze za běhu měnit zadními tlačítky) ===
    static bool isRed = true;          // Výchozí barva (true = RED, false = BLUE)
    static int layout_idx = 0;         // Výchozí kombinace docků (0 až 3)
    const float race_speed = 35.0f;    // Výchozí rychlost jízdy (%)

    // Globální/static pozice robota v ose X (sdílená lambdami a testovacími tlačítky)
    static float pos_x = isRed 
        ? (GameDimensions::FIELD_WIDTH_MM - GameDimensions::STARTZONE_CENTER_MM - GameDimensions::ZADEK_OD_STREDU_MM) 
        : GameDimensions::POCATECNI_X_STRED_MM;

    static bool start_requested = false;
    static bool has_switched_to_blue = false;
    static bool has_clicked_right = false;

    // Pokud ještě neodstartovalo, čteme konfiguraci
    if (!start_requested) {
        // Světelná indikace barvy týmu v pohotovostním režimu
        if (!has_switched_to_blue) {
            rkLedRed(true);
            rkLedGreen(false);
        } else {
            rkLedRed(false);
            rkLedGreen(true);
        }

        // Levé tlačítko: Pokud se klikalo pravým nebo už se přepnulo na BLUE -> START. Jinak přepne na BLUE.
        if (rkButtonLeft(false)) {
            delay(20); // Debounce
            if (rkButtonLeft(false)) {
                while (rkButtonLeft(false)) {
                    delay(10); // Čekání na uvolnění (release)
                }
                
                if (has_clicked_right || has_switched_to_blue) {
                    start_requested = true;
                    printf("[CONF] Stisk levého -> START PROGRAMU za %s!\n", isRed ? "RED" : "BLUE");
                } else {
                    has_switched_to_blue = true;
                    isRed = false;
                    pos_x = GameDimensions::POCATECNI_X_STRED_MM;
                    printf("[CONF] Přepnuto na BLUE (zelená LED) | Start X: %.0f mm\n", pos_x);
                }
                delay(100);
            }
        }

        // Pravé tlačítko: Mění kombinaci docků (0 až 3)
        if (rkButtonRight(false)) {
            delay(20); // Debounce
            if (rkButtonRight(false)) {
                while (rkButtonRight(false)) {
                    delay(10); // Čekání na uvolnění (release)
                }
                has_clicked_right = true;
                layout_idx = (layout_idx + 1) % 4;
                printf("[CONF] Pravé tlačítko uvolněno -> Kombinace docků: %d (has_clicked_right=true, levé tlačítko nyní startuje RED)\n", layout_idx);
                
                // Zpětná vazba: blikneme žlutou LED (index + 1)krát
                for (int i = 0; i <= layout_idx; i++) {
                    rkLedYellow(true);
                    delay(150);
                    rkLedYellow(false);
                    delay(150);
                }
            }
        }

        // Tlačítko ON: Okamžitý start pro aktuálně nastavenou barvu (zůstává jako záloha)
        if (rkButtonOn(false)) {
            delay(20);
            if (rkButtonOn(false)) {
                while (rkButtonOn(false)) {
                    delay(10);
                }
                start_requested = true;
                printf("[CONF] Tlačítko ON stisknuto -> START PROGRAMU za %s!\n", isRed ? "RED" : "BLUE");
            }
        }
    }

    // ================================================================
    // POMOCNÉ SOUTĚŽNÍ LAMBDY (přístupné pro všechna tlačítka)
    // ================================================================

    /**
     * Jede dopředu rychlostí power, dokud nenarazí předními tlačítky (bumpry).
     * Vrátí true, pokud úspěšně narazil, jinak false.
     */
    auto jed_do_narazu = [](float power, uint32_t timeout_ms) -> bool {
        printf("[BUMPER] Jedu dopředu rychlostí %.0f%% do nárazu...\n", power);
        rkMotorsSetSpeed(power, power);
        
        uint32_t start = millis();
        while (millis() - start < timeout_ms) {
            bool left = rkButtonLeft(false);
            bool right = rkButtonRight(false);
            
            if (left && right) {
                printf("[BUMPER] Obě tlačítka sepnuta (náraz čelní). Zastavuji.\n");
                rkMotorsSetSpeed(0, 0);
                return true;
            }
            if (left || right) {
                // Srovnání: jedno je sepnuté, chvilku ještě jedeme, aby se dotklo i druhé a srovnalo se
                delay(120);
                rkMotorsSetSpeed(0, 0);
                printf("[BUMPER] Dotyk detekován (L:%d, R:%d), srovnáno.\n", left, right);
                return true;
            }
            delay(10);
        }
        
        rkMotorsSetSpeed(0, 0);
        printf("[BUMPER] Timeout nárazu vypršel!\n");
        return false;
    };

    /**
     * Dojeď tak, aby rameno bylo nad cílovou X pozicí.
     * Automaticky počítá s offsetem ramena a směrem jízdy dle barvy týmu (pokud use_offset=true).
     */
    auto dojed_k = [&isRed](float& current_x, float target_arm_x, float spd = 35.0f, bool use_offset = true) -> bool {
        float center_target = target_arm_x;
        if (use_offset) {
            if (isRed) {
                // RED jede v -X: rameno je NAPRAVO od středu (vyšší X)
                center_target = target_arm_x - GameDimensions::RAMENO_OD_STREDU_MM;
            } else {
                // BLUE jede v +X: rameno je NALEVO od středu (nižší X)
                center_target = target_arm_x + GameDimensions::RAMENO_OD_STREDU_MM;
            }
        }
        float distance = isRed ? (current_x - center_target) : (center_target - current_x);
        printf("[DOJED] Z X=%.0f -> cíl ramene X=%.0f (střed=%.0f, offset=%s), vzdálenost=%.0f mm\n",
               current_x, target_arm_x, center_target, use_offset ? "ANO" : "NE", distance);
        
        uint32_t timeout = (uint32_t)(std::abs(distance) / 0.3f) + 3000; // dynamický timeout
        MoveResult res = move_straight_gyro(distance, spd, timeout, 0.0f);
        
        // Aktualizace pozice
        if (isRed) { current_x -= res.traveled_mm; }
        else       { current_x += res.traveled_mm; }
        printf("[DOJED] Nová pozice středu: X=%.0f mm\n", current_x);
        return res.success;
    };

    /**
     * Chyť baterku: otoč se k bateriím, seber ramenem, otoč zpět.
     * Končí s ramenem v pozici Up+Center (reset).
     */
    auto chyt_baterku = [](bool isRed, float original_bat_x, float& robot_x) {
        // Robot stojí stabilně na úhlu 0.0° (čelem k soupeři). 
        // Baterie jsou na pravé straně pro RED, na levé pro BLUE.
        int grab_side = isRed ? Rameno.iRight : Rameno.iLeft;
        int attempts = 0;
        float grab_base_angle = 0.0f; // Výchozí směr pro sběr je 0° (rovnoběžně s bateriemi)
        float current_target_angle = grab_base_angle;
        
        while (true) {
            // Po 5 neúspěšných pokusech (vyzkoušeny offsety 0°, 5°, -5°, 10°, -10°) popojedeme chytře v ose X
            if (attempts > 0 && attempts % 5 == 0) {
                printf("[GRAB] Baterka stále nenalezena (pokus %d). Srovnávám na 0° a popojedu v ose X...\n", attempts);
                turn_gyro(0.0f, 40);
                align_gyro(0.0f, 30);
                
                // Určíme velikost posunu: na 5. pokus o 40 mm, na 10. pokus o 80 mm
                float shift_magnitude = (attempts == 5) ? 40.0f : 80.0f;
                float shift = 0.0f;
                
                // Chytrý posun: pokud jsme za středem (X > 1500), jedeme směrem k 1500 (doleva).
                // Pro RED je jízda doleva dopředu (+shift), pro BLUE dozadu (-shift).
                if (isRed) {
                    shift = (robot_x > 1500.0f) ? shift_magnitude : -shift_magnitude;
                } else {
                    shift = (robot_x > 1500.0f) ? -shift_magnitude : shift_magnitude;
                }
                
                printf("[GRAB] Robot na X=%.1f. Popojíždím chytře o %.0f mm směrem ke středu...\n", robot_x, shift);
                MoveResult res = move_straight_gyro(shift, 20, 1500, 0.0f);
                if (isRed) {
                    robot_x -= res.traveled_mm;
                } else {
                    robot_x += res.traveled_mm;
                }
                printf("[GRAB] Nová pozice X: %.1f mm. Resetuji úhlové hledání...\n", robot_x);
                
                // Resetujeme cílový úhel zpět na 0° pro novou pozici
                current_target_angle = 0.0f;
            } else if (attempts > 0 && attempts % 10 == 0) {
                printf("[GRAB] Ani po 10 pokusech baterka nenalezena. Ukončuji hledání.\n");
                break;
            }

            printf("[GRAB] Pokus %d: Otáčím se na %.1f° k bateriím (strana %d, pozice X=%.1f)...\n", 
                   attempts + 1, current_target_angle, grab_side, robot_x);
            
            // Otáčení na požadovaný úhel (jemný offset od 0°)
            turn_gyro(current_target_angle, 40);
            align_gyro(current_target_angle, 30);
            
            Rameno.Side(grab_side);
            delay(800);
            Rameno.Down();
            delay(800);
            Rameno.Magnet(true);
            delay(600);
            Rameno.Up();
            delay(800);
            
            // Zkontrolovat laserem, jestli držíme baterku
            if (Rameno.HasBattery()) {
                printf("[GRAB] ÚSPĚCH! Baterka detekována na magnetu.\n");
                break;
            }
            
            printf("[GRAB] CHYBA: Baterka nedetekována! Uvolňuji magnet...\n");
            
            // Pustíme magnet pro další pokus
            Rameno.Magnet(false);
            delay(200);
            Rameno.Center();
            delay(300);
            
            attempts++;
            
            // Pootočíme se v rozšiřujícím se úhlovém vzoru (+5, -5, +10, -10)
            float offset = 0.0f;
            if (attempts % 5 == 1) offset = 5.0f;
            else if (attempts % 5 == 2) offset = -5.0f;
            else if (attempts % 5 == 3) offset = 10.0f;
            else if (attempts % 5 == 4) offset = -10.0f;
            
            current_target_angle = grab_base_angle + offset;
        }
        
        // Na konci se vždy srovnáme na 0.0°
        printf("[GRAB] Otáčím zpět na 0°...\n");
        turn_gyro(0.0f, 40);
        align_gyro(0.0f, 30);
        
        Rameno.Center();
        delay(300);
        printf("[GRAB] Funkce chyt_baterku dokončena.\n");
    };

    /**
     * Pusť baterku do docku: rameno na bok, dolů, pusť, zpět.
     * Končí s ramenem v pozici Up+Center (reset) a magnetem zapunutým (pro další baterku).
     */
    auto pust_baterku = [](bool isRed) {
        // RED: dock je vlevo od robota (iLeft), BLUE: dock je vpravo (iRight)
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
    // 1) START SOUTĚŽNÍ JÍZDY (vyvolaný zadním tlačítkem nebo ON)
    // ================================================================
    if (start_requested) {
        start_requested = false;      // Reset příznaku pro případnou další jízdu
        has_switched_to_blue = false; // Reset barvy pro další start
        has_clicked_right = false;    // Reset kliknutí pravým pro další start
        
        // Re-inicializace sdílené pozice pos_x pro novou jízdu
        if (isRed) {
            pos_x = GameDimensions::FIELD_WIDTH_MM - GameDimensions::STARTZONE_CENTER_MM 
                    - GameDimensions::ZADEK_OD_STREDU_MM; // 2720 mm
        } else {
            pos_x = GameDimensions::POCATECNI_X_STRED_MM; // 280 mm
        }

        printf("=== START SOUTĚŽNÍ JÍZDY ===\n");
        printf("Tým: %s | Kombinace: %d | Start X: %.0f mm\n", 
               isRed ? "ČERVENÝ" : "MODRÝ", layout_idx, pos_x);

        // Indikace + čekání 1 sekundu
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

        // === SLOUPCE BATERIÍ: každou z jiného sloupce, nejvzdálenější první (vybíráme 4 nejvzdálenější z 6 sloupců) ===
        float bat_cols_all[6] = {
            GameDimensions::BATTERY_COL1_X_MM, GameDimensions::BATTERY_COL2_X_MM,
            GameDimensions::BATTERY_COL3_X_MM, GameDimensions::BATTERY_COL4_X_MM,
            GameDimensions::BATTERY_COL5_X_MM, GameDimensions::BATTERY_COL6_X_MM
        };
        // Seřazení všech 6 sloupců podle vzdálenosti: nejvzdálenější od startu první
        for (int i = 0; i < 5; i++) {
            for (int j = i + 1; j < 6; j++) {
                float di = std::abs(bat_cols_all[i] - pos_x);
                float dj = std::abs(bat_cols_all[j] - pos_x);
                if (dj > di) {
                    float tmp = bat_cols_all[i];
                    bat_cols_all[i] = bat_cols_all[j];
                    bat_cols_all[j] = tmp;
                }
            }
        }
        // Vezmeme první 4 nejvzdálenější sloupce
        float bat_cols_sorted[4] = {
            bat_cols_all[0], bat_cols_all[1],
            bat_cols_all[2], bat_cols_all[3]
        };

        printf("Pořadí docků: ");
        for (int i = 0; i < dock_count; i++) printf("%d(%.0f) ", my_docks[i]+1, GameDimensions::DOCK_X_POSITIONS[my_docks[i]]);
        printf("\nPořadí baterií: ");
        for (int i = 0; i < 4; i++) printf("%.0f ", bat_cols_sorted[i]);
        printf("\n");

        // === HLAVNÍ SMYČKA: 4× seber baterku a vlož do docku ===
        int delivered = 0;
        bool dock_delivered[4] = {false, false, false, false};

        for (int cycle = 0; cycle < 4 && cycle < dock_count; cycle++) {
            printf("\n======== CYKLUS %d/4 ========\n", cycle + 1);
            
            // LED indikace cyklu
            rkLedGreen(true);
            
            // 1. Dojeď k baterii (přímo k cílovému sloupci s offsetem ramene)
            float original_bat_x = bat_cols_sorted[cycle];
            float bat_x = original_bat_x;
            
            // Nekonečná smyčka pro bezpečné dojetí k baterii (pokud překáží soupeř, počkáme 5s a zkusíme znovu)
            while (true) {
                printf("[%d] Jedu k baterii na X=%.0f...\n", cycle+1, bat_x);
                if (dojed_k(pos_x, bat_x, race_speed, true)) {
                    // Úspěšně dojeto k baterii!
                    break;
                }
                
                printf("[%d] Cesta k baterii zablokována soupeřem! Blikám červeně a čekám 5 sekund...\n", cycle+1);
                // Blikání červenou LEDkou po dobu 5 sekund (25× 200 ms)
                for (int i = 0; i < 25; i++) {
                    rkLedRed(true);
                    delay(100);
                    rkLedRed(false);
                    delay(100);
                }
                printf("[%d] Zkouším jet k baterii znovu...\n", cycle+1);
            }
            delay(200);
            
            // 2. Chyť baterku
            rkLedYellow(true);
            chyt_baterku(isRed, original_bat_x, pos_x);
            rkLedGreen(false);
            delay(200);
            
            // 3. Dojeď k docku (zkoušíme od nejvzdálenějšího 'cycle', při selhání zkusíme bližší)
            bool success = false;
            int chosen_dock_idx = -1;
            
            for (int d_idx = cycle; d_idx < 4 && d_idx < dock_count; d_idx++) {
                if (dock_delivered[d_idx]) continue; // Do tohoto docku už bylo doručeno
                
                float dock_x = GameDimensions::DOCK_X_POSITIONS[my_docks[d_idx]];
                printf("[%d] Zkouším jet k docku %d na X=%.0f...\n", cycle+1, my_docks[d_idx]+1, dock_x);
                rkLedRed(true);
                
                if (dojed_k(pos_x, dock_x, race_speed)) {
                    success = true;
                    chosen_dock_idx = d_idx;
                    break;
                } else {
                    printf("[%d] Cesta k docku %d zablokována soupeřem! Blikám červeně a zkusím bližší...\n", 
                           cycle+1, my_docks[d_idx]+1);
                    // Zablikat červenou LEDkou
                    for(int i=0; i<5; i++) {
                        rkLedRed(true); delay(80); rkLedRed(false); delay(80);
                    }
                    delay(200);
                }
            }
            
            if (success && chosen_dock_idx != -1) {
                // 4. Pusť baterku
                pust_baterku(isRed);
                dock_delivered[chosen_dock_idx] = true;
                delivered++;
                rkLedAll(false);
                printf("[%d] Baterka %d doručena do docku %d! (%d/4)\n", 
                       cycle+1, cycle+1, my_docks[chosen_dock_idx]+1, delivered);
            } else {
                printf("[%d] Nelze doručit do žádného volného docku kvůli soupeři! Uvolňuji rameno...\n", cycle+1);
                // Nouzové uvolnění baterky na naší straně, aby se nezaseklo rameno
                Rameno.Side(isRed ? Rameno.iLeft : Rameno.iRight);
                delay(800);
                Rameno.Magnet(false);
                delay(500);
                Rameno.Up();
                delay(800);
                Rameno.Center();
                delay(300);
                Rameno.Magnet(true); // nachystat pro příště
                rkLedAll(false);
            }
            delay(200);
        }

        // === NÁVRAT DOMŮ: Otočení a parkovací manévr přes nárazové bumpry ===
        printf("\n=== NÁVRAT DOMŮ (PARKOVÁNÍ) ===\n");
        rkLedRed(true); rkLedGreen(true);
        
        // 1. Dorovnat směr k soupeři (předek míří k soupeři)
        align_gyro(0.0f, 30);
        
        // 2. Zacouvat zadkem robota k domácí zdi (bez otáčení o 180°)
        float home_x = isRed 
            ? (GameDimensions::FIELD_WIDTH_MM - 50.0f)   // Pravá zeď
            : 50.0f;                                       // Levá zeď
        float back_distance = std::abs(pos_x - home_x) + 120.0f; // + 120mm pro jistotu nárazu zadku
        printf("[HOME] Couvám %.0f mm ke zdi (zadek napřed)...\n", back_distance);
        move_straight_gyro(-back_distance, 25, 8000, 0.0f);
        delay(200);
        
        // 3. Popojet dopředu (do hřiště) o 100 mm (10 cm)
        printf("[HOME] Popojíždím dopředu o 100 mm...\n");
        move_straight_gyro(100.0f, 25, 2000, 0.0f);
        delay(200);
        
        // 4. Otočit se o 90° doprava k bočnímu mantinelu (RED doprava -90.0, BLUE doleva 90.0)
        float wall_angle = isRed ? -90.0f : 90.0f;
        printf("[HOME] Otáčím se k bočnímu mantinelu (úhel %.1f°)...\n", wall_angle);
        turn_gyro(wall_angle, 40);
        align_gyro(wall_angle, 30);
        
        // 5. Jízda DOZADU (couvání) do nárazu zadními bumpry do bočního mantinelu
        printf("[HOME] Couvám do nárazu zadními bumpry do mantinelu...\n");
        jed_do_narazu(-20.0f, 4000);
        delay(200);
        
        // 6. Popojet dopředu o 300 mm (30 cm) zpět od mantinelu
        printf("[HOME] Odtahuji se od mantinelu (jedou dopředu 300 mm)...\n");
        move_straight_gyro(300.0f, 25, 3000, wall_angle);
        delay(200);
        
        // 7. Otočit se tak, aby zadek mířil k domácí zdi (úhel 0.0°)
        printf("[HOME] Otáčím se k domácí zdi (úhel 0.0°)...\n");
        turn_gyro(0.0f, 40);
        align_gyro(0.0f, 30);
        
        // 8. Finální jemný nájezd zadkem do domácí zdi (zadní bumpry)
        printf("[HOME] Finální usazení zadkem do domácí zdi...\n");
        jed_do_narazu(-15.0f, 3000);
        
        // Stop
        rkMotorsSetSpeed(0, 0);
        delay(300);

        // === KONEC ZÁPASU: Zvukový signál a blikání ===
        printf("=== KONEC ZÁPASU === Doručeno: %d/4 baterií\n", delivered);
        for (int i = 0; i < 8; i++) {
            rkLedRed(i % 2 == 0); rkLedYellow(i % 2 == 1); rkLedGreen(i % 2 == 0);
            delay(200);
            rkLedAll(false);
            delay(100);
        }
        isRed = true; // Nastavíme zpět na výchozí RED pro příští běhy
    }

    // ================================================================
    // 2) TLAČÍTKO UP: TEST JÍZDY ROVNĚ A ZPĚT
    // ================================================================
    if (rkButtonUp(true)) {
        printf("=== UP: TEST JÍZDY ROVNĚ (500 mm tam a zpět) ===\n");
        rkLedGreen(true);
        
        // Jízda dopředu o 500 mm se stabilizací k úhlu 0.0°
        move_straight_gyro(500.0f, race_speed, 4000, 0.0f);
        delay(1000);
        
        // Jízda dozadu (zpět) o 500 mm
        move_straight_gyro(-500.0f, race_speed, 4000, 0.0f);
        
        rkLedGreen(false);
        printf("=== TEST JÍZDY DOKONČEN ===\n");
    }

    // ================================================================
    // 3) TLAČÍTKO DOWN: TEST JÍZDY S DETEKCÍ SOUPEŘE A PÍSKÁNÍM
    // ================================================================
    if (rkButtonDown(true)) {
        printf("=== DOWN: TEST JÍZDY ROVNĚ S DETEKCÍ SOUPEŘE (800 mm) ===\n");
        rkLedYellow(true);
        
        // Jízda dopředu o 800 mm se stabilizací k úhlu 0.0° a detekcí soupeře
        MoveResult res = move_straight_gyro(800.0f, race_speed, 6000, 0.0f);
        
        if (!res.success) {
            // Detekován soupeř a okamžitě zastaveno
            printf("[DOWN] DETEKOVÁN SOUPEŘ! Okamžité zastavení.\n");
        } else {
            printf("[DOWN] Jízda dokončena úspěšně bez soupeře.\n");
        }
        
        rkLedYellow(false);
        printf("=== TEST JÍZDY DOKONČEN ===\n");
    }



    // ================================================================
    // 6) TLAČÍTKO OFF: VYPISOVÁNÍ ULTRAZVUKŮ (CO 0.5s)
    // ================================================================
    if (rkButtonOff(true)) {
        printf("=== OFF: SPUŠTĚNO VYPISOVÁNÍ ULTRAZVUKŮ ===\n");
        printf("(Stiskněte libovolné tlačítko pro ukončení)\n");
        rkLedRed(true); rkLedYellow(true); rkLedGreen(true);
        
        // Čekání na uvolnění tlačítka OFF, aby se smyčka ihned neukončila
        while (rkButtonIsPressed(BTN_OFF)) {
            delay(10);
        }
        delay(200);
        rkLedAll(false);

        uint32_t last_print = 0;
        while (true) {
            // Ukončení stiskem jakéhokoli tlačítka
            if (rkButtonIsPressed(BTN_ON) || rkButtonIsPressed(BTN_OFF) || 
                rkButtonIsPressed(BTN_UP) || rkButtonIsPressed(BTN_DOWN) || 
                rkButtonIsPressed(BTN_LEFT) || rkButtonIsPressed(BTN_RIGHT)) {
                break;
            }
            
            uint32_t now = millis();
            if (now - last_print >= 500) {
                uint32_t u1 = uz_predni();
                uint32_t u2 = uz_predni_levy();
                uint32_t u3 = uz_predni_pravy();
                uint32_t u4 = uz_zadek();
                int laser = uz_laser();
                bool has_bat = Rameno.HasBattery();
                printf("U1 (predni): %4u mm | U2 (predni levy): %4u mm | U3 (predni pravy): %4u mm | U4 (zadek): %4u mm | LASER: ", u1, u2, u3, u4);
                if (laser >= 0) {
                    printf("%4d mm", laser);
                } else {
                    printf(" Err");
                }
                printf(" | BATTERY: %s\n", has_bat ? "MAME" : "NEMAME");
                last_print = now;
            }
            delay(10);
        }
        
        // Krátké bliknutí pro indikaci ukončení
        rkLedRed(true);
        delay(300);
        rkLedRed(false);
        printf("=== VYPISOVÁNÍ ULTRAZVUKŮ UKONČENO ===\n");
    }

    delay(10);
}

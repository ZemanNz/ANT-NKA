#include "robotka.h"
#include <SmartLeds.h>

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

// Globální instance RGB LED pásku (8 diod, pin 12)
SmartLed strip(LED_WS2812B, 8, 12, 0, DoubleBuffer);

// Pomocná funkce pro aktualizaci LED pásku
void aktualizuj_led_pasek(bool isRed, int layout_idx) {
    // Získání barev z kombinace docků
    const TeamColor* layout = GameDimensions::DOCK_LAYOUTS[layout_idx];
    
    for (int i = 0; i < 8; i++) {
        // Index baterky: pro RED odspodu 7->0, pro BLUE odspodu 0->7
        int battery_idx = isRed ? (7 - i) : i;
        TeamColor color = layout[battery_idx];
        
        if (color == TeamColor::Red) {
            strip[i] = Rgb{2, 0, 0};
        } else if (color == TeamColor::Blue) {
            strip[i] = Rgb{0, 0, 2};
        } else {
            strip[i] = Rgb{0, 0, 0};
        }
    }
    strip.show();
}


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
    static int layout_idx = 0;         // Výchozí kombinace docků (index 0, tj. kombinace 1)
    const float race_speed = 35.0f;    // Výchozí rychlost jízdy (%)

    // Globální/static pozice robota v ose X (sdílená lambdami a testovacími tlačítky)
    static float pos_x = isRed 
        ? (GameDimensions::FIELD_WIDTH_MM - GameDimensions::STARTZONE_CENTER_MM - GameDimensions::ZADEK_OD_STREDU_MM) 
        : GameDimensions::POCATECNI_X_STRED_MM;

    static bool start_requested = false;

    // Pokud ještě neodstartovalo, čteme konfiguraci
    if (!start_requested) {
        // Aktualizace LED pásku (červená vs modrá, výška podle kombinace)
        aktualizuj_led_pasek(isRed, layout_idx);

        // Světelná indikace barvy týmu v pohotovostním režimu
        if (isRed) {
            rkLedRed(true);
            rkLedBlue(false);
            rkLedGreen(false);
        } else {
            rkLedRed(false);
            rkLedBlue(true);
            rkLedGreen(false);
        }

        // Tlačítko UP: Výběr barvy týmu RED <-> BLUE
        if (rkButtonUp(false)) {
            delay(20); // Debounce
            if (rkButtonUp(false)) {
                while (rkButtonUp(false)) {
                    delay(10);
                }
                isRed = !isRed;
                pos_x = isRed 
                    ? (GameDimensions::FIELD_WIDTH_MM - GameDimensions::STARTZONE_CENTER_MM - GameDimensions::ZADEK_OD_STREDU_MM) 
                    : GameDimensions::POCATECNI_X_STRED_MM;
                printf("[CONF] Tlačítko UP uvolněno -> Přepnuto na %s | Start X: %.0f mm\n", 
                       isRed ? "RED (Červená LED)" : "BLUE (Modrá LED)", pos_x);
                delay(100);
            }
        }

        // Levé tlačítko: Krátký stisk -> Kombinace 3 (start), Dlouhý stisk -> Kombinace 4 (start)
        if (rkButtonLeft(false)) {
            delay(20); // Debounce
            if (rkButtonLeft(false)) {
                uint32_t press_start = millis();
                while (rkButtonLeft(false)) {
                    delay(10);
                }
                uint32_t duration = millis() - press_start;
                if (duration > 500) {
                    layout_idx = 3; // Kombinace 4
                    printf("[CONF] Levé tlačítko uvolněno (DLOUHÝ stisk: %lu ms) -> START (Kombinace 4)\n", (unsigned long)duration);
                } else {
                    layout_idx = 2; // Kombinace 3
                    printf("[CONF] Levé tlačítko uvolněno (KRÁTKÝ stisk: %lu ms) -> START (Kombinace 3)\n", (unsigned long)duration);
                }
                start_requested = true;
                delay(100);
            }
        }

        // Pravé tlačítko: Krátký stisk -> Kombinace 1 (start), Dlouhý stisk -> Kombinace 2 (start)
        if (rkButtonRight(false)) {
            delay(20); // Debounce
            if (rkButtonRight(false)) {
                uint32_t press_start = millis();
                while (rkButtonRight(false)) {
                    delay(10);
                }
                uint32_t duration = millis() - press_start;
                if (duration > 500) {
                    layout_idx = 1; // Kombinace 2
                    printf("[CONF] Pravé tlačítko uvolněno (DLOUHÝ stisk: %lu ms) -> START (Kombinace 2)\n", (unsigned long)duration);
                } else {
                    layout_idx = 0; // Kombinace 1
                    printf("[CONF] Pravé tlačítko uvolněno (KRÁTKÝ stisk: %lu ms) -> START (Kombinace 1)\n", (unsigned long)duration);
                }
                start_requested = true;
                delay(100);
            }
        }

        // Tlačítko DOWN: Pouze cyklické přepínání kombinací (0 až 3) pro náhled na LED pásku
        if (rkButtonDown(false)) {
            delay(20); // Debounce
            if (rkButtonDown(false)) {
                while (rkButtonDown(false)) {
                    delay(10);
                }
                layout_idx = (layout_idx + 1) % 4;
                printf("[CONF] Tlačítko DOWN uvolněno -> Změna kombinace pro náhled: %d\n", layout_idx + 1);
                
                // Zpětná vazba: blikneme žlutou LED (index + 1)krát
                for (int i = 0; i <= layout_idx; i++) {
                    rkLedYellow(true);
                    delay(150);
                    rkLedYellow(false);
                    delay(150);
                }
            }
        }
    }

    // ================================================================
    // POMOCNÉ SOUTĚŽNÍ LAMBDY (přístupné pro všechna tlačítka)
    // ================================================================


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

    auto chyt_baterku = [](bool isRed, float original_bat_x, float& robot_x) {
        // Robot stojí stabilně na úhlu 0.0° (čelem k soupeři). 
        // Baterie jsou na pravé straně pro RED, na levé pro BLUE.
        int grab_side = isRed ? Rameno.iRight : Rameno.iLeft;
        int attempts = 0;
        float current_target_angle = 0.0f;
        bool arm_is_on_side = false;
        
        while (true) {
            // Po 2 neúspěšných pokusech (vyzkoušen úhel 0° a 7°) popojedeme chytře v ose X
            if (attempts > 0 && attempts % 2 == 0) {
                printf("[GRAB] Baterka stále nenalezena (pokus %d). Srovnávám na 0° a popojedu v ose X...\n", attempts);
                turn_gyro(0.0f, 40);
                align_gyro(0.0f, 30);
                
                // Určíme velikost posunu: konstantních 40 mm od předchozí pozice
                float shift_magnitude = 40.0f;
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
            } else if (attempts > 0 && attempts % 6 == 0) {
                printf("[GRAB] Ani po 6 pokusech baterka nenalezena. Ukončuji hledání.\n");
                break;
            }

            printf("[GRAB] Pokus %d: Otáčím se na %.1f° k bateriím (strana %d, pozice X=%.1f)...\n", 
                   attempts + 1, current_target_angle, grab_side, robot_x);
            
            // Otáčení na požadovaný úhel (jemný offset od 0°)
            turn_gyro(current_target_angle, 40);
            align_gyro(current_target_angle, 30);
            
            if (!arm_is_on_side) {
                Rameno.Side(grab_side);
                delay(800);
                arm_is_on_side = true;
            } else {
                // Pokud už je rameno na boku, stačí kratičká stabilizační prodleva po otočení robota
                delay(100);
            }
            
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
            
            attempts++;
            
            // Pootočíme se: na druhém pokusu na daném místě zkusíme 7° (střídáme směr otočení při každém popojetí)
            float offset = 0.0f;
            if (attempts % 2 == 1) {
                // Zůstáváme na místě (jen se pootočíme), rameno necháme na boku
                offset = ((attempts / 2) % 2 == 0) ? 7.0f : -7.0f;
            } else {
                // Budeme popojíždět, takže uklidíme rameno doprostřed
                Rameno.Center();
                delay(300);
                arm_is_on_side = false;
            }
            
            current_target_angle = offset;
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
        
        // Prodleva 300 ms po uvolnění startovního tlačítka (pro klidné oddálení ruky)
        delay(300);

        // Zhasnutí LED pásku při startu jízdy
        for (int i = 0; i < 8; i++) {
            strip[i] = Rgb{0, 0, 0};
        }
        strip.show();
        
        // === POMOCNÝ TEST BEZ POHYBU (na přání uživatele) ===
        bool disable_movement = false;
        if (disable_movement) {
            printf("[TEST] Pohyb zakázán. Zhasínám LED pásek a vracím se zpět do menu za 3 sekundy.\n");
            delay(3000);
            isRed = true; // Nastavíme zpět na RED
            return; // Ukončíme loop() předčasně pro vynechání pohybu
        }
        
        // Re-inicializace sdílené pozice pos_x pro novou jízdu
        if (isRed) {
            pos_x = GameDimensions::FIELD_WIDTH_MM - GameDimensions::STARTZONE_CENTER_MM 
                    - GameDimensions::ZADEK_OD_STREDU_MM; // 2720 mm
        } else {
            pos_x = GameDimensions::POCATECNI_X_STRED_MM; // 280 mm
        }
        const float start_x = pos_x;

        printf("=== START SOUTĚŽNÍ JÍZDY ===\n");
        printf("Tým: %s | Kombinace: %d | Start X: %.0f mm\n", 
               isRed ? "ČERVENÝ" : "MODRÝ", layout_idx + 1, pos_x);

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
        // Vynecháme ten úplně nejvzdálenější (bat_cols_all[0]) a vezmeme další 4 v pořadí od druhého nejvzdálenějšího
        float bat_cols_sorted[4] = {
            bat_cols_all[1], bat_cols_all[2],
            bat_cols_all[3], bat_cols_all[4]
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
            if (delivered >= 3) {
                printf("[CYCLE] Již byly úspěšně doručeny 3 baterky. Končím cyklus dříve.\n");
                break;
            }
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

        // === NÁVRAT DOMŮ: Dojetí na startovní pozici X ===
        printf("\n=== NÁVRAT DOMŮ (PARKOVÁNÍ) ===\n");
        rkLedRed(true); rkLedGreen(true);
        
        // 1. Dorovnat směr k soupeři (předek míří k soupeři)
        align_gyro(0.0f, 30);
        
        // 2. Dojet na počáteční pozici X
        printf("[HOME] Jedu na počáteční pozici X (%.0f mm)...\n", start_x);
        dojed_k(pos_x, start_x, race_speed, false);
        
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

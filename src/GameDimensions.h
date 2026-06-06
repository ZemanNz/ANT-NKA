#pragma once

// Definice barev týmů
enum class TeamColor {
    Blue,
    Red,
    Unknown
};

// Struktura pro 2D pozici (v milimetrech)
struct Position2D {
    float x;
    float y;
};

namespace GameDimensions {
    // =========================================================================
    // ROZMĚRY HŘIŠTĚ & HERNÍCH PRVKŮ (dle oficiálních pravidel)
    // =========================================================================
    constexpr float FIELD_WIDTH_MM = 3000.0f;          // Šířka herní plochy: 3 m
    constexpr float FIELD_HEIGHT_MM = 2000.0f;         // Délka herní plochy: 2 m
    constexpr float BORDER_HEIGHT_MM = 70.0f;          // Výška mantinelu: 7 cm
    constexpr float START_ZONE_SIZE_MM = 500.0f;       // Startovní zóna: 50x50 cm

    // Fyzické rozměry prvků
    constexpr float BATTERY_SIZE_MM = 60.0f;           // Kostka baterie: hrana 6 cm
    constexpr float BATTERY_RING_DIAMETER_MM = 40.0f;  // Feromagnetický kroužek: cca 4 cm
    constexpr float CARGO_SIZE_MM = 60.0f;             // Kostka nákladu: hrana 6 cm

    // Auta pro baterie (Docky) - naměřeno
    constexpr float DOCK_WIDTH_MM = 120.0f;            // Šířka boxu (auta): 12 cm
    constexpr float DOCK_GAP_MM = 80.0f;               // Mezera hrana-hrana mezi boxy: 8 cm
    constexpr float DOCK_PITCH_MM = DOCK_WIDTH_MM + DOCK_GAP_MM; // Rozteč střed-střed: 200 mm

    constexpr float CAR_LENGTH_MM = 220.0f;            // Délka auta: cca 22 cm
    constexpr float CAR_BACK_WALL_HEIGHT_MM = 60.0f;   // Výška zadní stěny: 6 cm
    constexpr float CAR_BATTERY_BAY_SIZE_MM = 80.0f;   // Místo pro baterii: min. 8x8 cm

    // Náhradní vozidlo pro náklad
    constexpr float TRUCK_BED_WIDTH_MM = 150.0f;       // Šířka ložné plochy: 15 cm
    constexpr float TRUCK_BED_LENGTH_MM = 240.0f;      // Délka ložné plochy: 24 cm
    constexpr float TRUCK_BED_HEIGHT_MM = 60.0f;       // Výška ložné plochy: 6 cm

    // Dopravní značka
    constexpr float SIGN_BASE_DIAMETER_MM = 75.0f;     // Průměr základny značky: cca 7.5 cm
    constexpr float SIGN_BASE_HEIGHT_MM = 25.0f;       // Výška základny: cca 2.5 cm

    // =========================================================================
    // GEOMETRIE ROBOTA (offsety měřeny od středu robota = střed osy kol)
    // =========================================================================
    //
    //   [ZADEK] ---80mm--- [STŘED/OSA KOL] ----> PŘEDEK
    //                  ^
    //                  |
    //              40mm od středu (mezi středem a zadkem)
    //              = bod RAMENE (kde rameno sahá k zemi na bok)
    //
    constexpr float ZADEK_OD_STREDU_MM = 80.0f;        // Zadek robota je 8 cm za středem
    constexpr float RAMENO_OD_STREDU_MM = 40.0f;       // Rameno je 4 cm za středem (mezi středem a zadkem)

    // =========================================================================
    // POČÁTEČNÍ POZICE ROBOTA
    // =========================================================================
    // Zadek robota je nově na X=20 cm (200 mm) od stěny (o 5 cm blíže stěně než původních 25 cm)
    constexpr float STARTZONE_CENTER_MM = 200.0f;                                 // 200 mm
    constexpr float POCATECNI_X_STRED_MM = STARTZONE_CENTER_MM + ZADEK_OD_STREDU_MM; // 280 mm
    constexpr float POCATECNI_X_RAMENO_MM = POCATECNI_X_STRED_MM - RAMENO_OD_STREDU_MM; // 240 mm

    // =========================================================================
    // POZICE BOXŮ PRO BATERIE (DOCKY) - naměřeno na hřišti
    // =========================================================================
    // Měření od boční zdi (krátká strana, kde startujeme):
    //   - Hrana prvního boxu od zdi: 750 mm
    //   - Šířka boxu: 120 mm  →  střed 1. boxu = 750 + 60 = 810 mm
    //   - Mezera hrana-hrana: 80 mm  →  rozteč střed-střed: 200 mm
    //
    //   Zdi    750    870  950   1070  1150  1270  1350  1470  1550  1670  1750  1870  1950  2070  2150  2270
    //    |------[===BOX 1===]--80--[===BOX 2===]--80--[===BOX 3===]--80--[===BOX 4===]--80--[===BOX 5===]--...
    //              810          1010          1210          1410          1610          1810  ...

    constexpr float DOCK_FIRST_EDGE_MM = 750.0f;       // Hrana 1. boxu od zdi
    constexpr float DOCK_FIRST_CENTER_MM = DOCK_FIRST_EDGE_MM + DOCK_WIDTH_MM / 2.0f; // 810 mm

    // Středy 8 boxů (docků) na ose X [mm od zdi]
    const float DOCK_X_POSITIONS[8] = {
        DOCK_FIRST_CENTER_MM + 0 * DOCK_PITCH_MM,  //  810 mm - Dock 1
        DOCK_FIRST_CENTER_MM + 1 * DOCK_PITCH_MM,  // 1010 mm - Dock 2
        DOCK_FIRST_CENTER_MM + 2 * DOCK_PITCH_MM,  // 1210 mm - Dock 3
        DOCK_FIRST_CENTER_MM + 3 * DOCK_PITCH_MM,  // 1410 mm - Dock 4
        DOCK_FIRST_CENTER_MM + 4 * DOCK_PITCH_MM,  // 1610 mm - Dock 5
        DOCK_FIRST_CENTER_MM + 5 * DOCK_PITCH_MM,  // 1810 mm - Dock 6
        DOCK_FIRST_CENTER_MM + 6 * DOCK_PITCH_MM,  // 2010 mm - Dock 7
        DOCK_FIRST_CENTER_MM + 7 * DOCK_PITCH_MM   // 2210 mm - Dock 8
    };

    // Y pozice docků na hřišti (vzdálenost od přední dlouhé stěny)
    constexpr float DOCK_Y_POSITION = 100.0f;

    // =========================================================================
    // CÍLOVÉ POZICE STŘEDU ROBOTA PRO DOCKY
    // =========================================================================
    // Rameno je ZA středem robota o RAMENO_OD_STREDU_MM.
    // Aby rameno bylo nad středem docku, střed robota musí PŘEJET o offset.
    //   cíl_střed = dock_X + RAMENO_OD_STREDU_MM
    const float DOCK_TARGETS[8] = {
        DOCK_FIRST_CENTER_MM + 0 * DOCK_PITCH_MM + RAMENO_OD_STREDU_MM,  //  850 mm
        DOCK_FIRST_CENTER_MM + 1 * DOCK_PITCH_MM + RAMENO_OD_STREDU_MM,  // 1050 mm
        DOCK_FIRST_CENTER_MM + 2 * DOCK_PITCH_MM + RAMENO_OD_STREDU_MM,  // 1250 mm
        DOCK_FIRST_CENTER_MM + 3 * DOCK_PITCH_MM + RAMENO_OD_STREDU_MM,  // 1450 mm
        DOCK_FIRST_CENTER_MM + 4 * DOCK_PITCH_MM + RAMENO_OD_STREDU_MM,  // 1650 mm
        DOCK_FIRST_CENTER_MM + 5 * DOCK_PITCH_MM + RAMENO_OD_STREDU_MM,  // 1850 mm
        DOCK_FIRST_CENTER_MM + 6 * DOCK_PITCH_MM + RAMENO_OD_STREDU_MM,  // 2050 mm
        DOCK_FIRST_CENTER_MM + 7 * DOCK_PITCH_MM + RAMENO_OD_STREDU_MM   // 2250 mm
    };

    // =========================================================================
    // POZICE BATERIÍ - naměřeno na hřišti (12 baterek v 6 sloupcích)
    // =========================================================================
    // Středy 6 sloupců baterií jsou symetrické kolem středu hřiště X = 1500 mm.
    // Šířka baterie = 60 mm, rozteč (střed-střed) = 60 mm (sloupce stojí těsně vedle sebe).
    constexpr float BATTERY_COL1_X_MM = 1350.0f;
    constexpr float BATTERY_COL2_X_MM = 1410.0f;
    constexpr float BATTERY_COL3_X_MM = 1470.0f;
    constexpr float BATTERY_COL4_X_MM = 1530.0f;
    constexpr float BATTERY_COL5_X_MM = 1590.0f;
    constexpr float BATTERY_COL6_X_MM = 1650.0f;

    constexpr float BATTERY_ROW_CLOSER_Y_MM = 950.0f;  // Bližší k autům
    constexpr float BATTERY_ROW_FARTHER_Y_MM = 1050.0f; // Vzdálenější od aut

    // Fyzické pozice 12 baterií (Sloupec × Řada)
    const Position2D BATTERIES[12] = {
        {BATTERY_COL1_X_MM, BATTERY_ROW_CLOSER_Y_MM},   // Baterie 1 (Sl.1, blíž)
        {BATTERY_COL1_X_MM, BATTERY_ROW_FARTHER_Y_MM},  // Baterie 2 (Sl.1, dál)
        {BATTERY_COL2_X_MM, BATTERY_ROW_CLOSER_Y_MM},   // Baterie 3 (Sl.2, blíž)
        {BATTERY_COL2_X_MM, BATTERY_ROW_FARTHER_Y_MM},  // Baterie 4 (Sl.2, dál)
        {BATTERY_COL3_X_MM, BATTERY_ROW_CLOSER_Y_MM},   // Baterie 5 (Sl.3, blíž)
        {BATTERY_COL3_X_MM, BATTERY_ROW_FARTHER_Y_MM},  // Baterie 6 (Sl.3, dál)
        {BATTERY_COL4_X_MM, BATTERY_ROW_CLOSER_Y_MM},   // Baterie 7 (Sl.4, blíž)
        {BATTERY_COL4_X_MM, BATTERY_ROW_FARTHER_Y_MM},  // Baterie 8 (Sl.4, dál)
        {BATTERY_COL5_X_MM, BATTERY_ROW_CLOSER_Y_MM},   // Baterie 9 (Sl.5, blíž)
        {BATTERY_COL5_X_MM, BATTERY_ROW_FARTHER_Y_MM},  // Baterie 10 (Sl.5, dál)
        {BATTERY_COL6_X_MM, BATTERY_ROW_CLOSER_Y_MM},   // Baterie 11 (Sl.6, blíž)
        {BATTERY_COL6_X_MM, BATTERY_ROW_FARTHER_Y_MM}   // Baterie 12 (Sl.6, dál)
    };

    // Cílové pozice středu robota pro sběr baterií pro všech 6 sloupců
    const float BATTERY_TARGETS[6] = {
        BATTERY_COL1_X_MM + RAMENO_OD_STREDU_MM,
        BATTERY_COL2_X_MM + RAMENO_OD_STREDU_MM,
        BATTERY_COL3_X_MM + RAMENO_OD_STREDU_MM,
        BATTERY_COL4_X_MM + RAMENO_OD_STREDU_MM,
        BATTERY_COL5_X_MM + RAMENO_OD_STREDU_MM,
        BATTERY_COL6_X_MM + RAMENO_OD_STREDU_MM
    };

    // =========================================================================
    // 4 KOMBINACE ROZLOŽENÍ DOCKŮ (Barevné boxy aut zleva doprava)
    // =========================================================================
    // Definováno z pohledu MODRÉHO týmu (pokud jste červený tým, barvy se invertují)
    const TeamColor DOCK_LAYOUTS[4][8] = {
        // Kombinace 0: Blue je 1., 5., 6., 7.
        {TeamColor::Blue, TeamColor::Red, TeamColor::Red, TeamColor::Red,
         TeamColor::Blue, TeamColor::Blue, TeamColor::Blue, TeamColor::Red},

        // Kombinace 1: Blue je 2., 5., 6., 8.
        {TeamColor::Red, TeamColor::Blue, TeamColor::Red, TeamColor::Red,
         TeamColor::Blue, TeamColor::Blue, TeamColor::Red, TeamColor::Blue},

        // Kombinace 2: Blue je 3., 5., 7., 8.
        {TeamColor::Red, TeamColor::Red, TeamColor::Blue, TeamColor::Red,
         TeamColor::Blue, TeamColor::Red, TeamColor::Blue, TeamColor::Blue},

        // Kombinace 3: Blue je 4., 6., 7., 8.
        {TeamColor::Red, TeamColor::Red, TeamColor::Red, TeamColor::Blue,
         TeamColor::Red, TeamColor::Blue, TeamColor::Blue, TeamColor::Blue}
    };

    // =========================================================================
    // POMOCNÁ FUNKCE: Vzdálenost pro střed robota ze současné pozice k cíli
    // =========================================================================
    // Vrátí kolik mm musí střed robota ujet, aby se rameno dostalo na cílovou X pozici
    inline float vzdalenostKCili(float currentCenterX, float targetArmX) {
        return (targetArmX + RAMENO_OD_STREDU_MM) - currentCenterX;
    }
}

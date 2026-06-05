#pragma once

#include <Arduino.h>
#include <algorithm>
#include <functional>
#include <cmath>
#include "robotka.h"
#include "_librk_context.h"

struct MoveResult {
    bool success;
    float traveled_mm; // Skutečně ujetá vzdálenost se znaménkem (+ vpřed, - vzad)
};

/**
 * \brief Plynulý pohyb s vyhýbáním se (robot nenarazí).
 * 
 * \param mm Vzdálenost v milimetrech (kladná pro jízdu vpřed, záporná pro vzad).
 * \param speed Rychlost v % (0-100).
 * \param is_obstacle Funkce/lambda, která vrátí true, pokud je detekována překážka.
 * \param wait_timeout_ms Jak dlouho (v ms) robot maximálně počká, než vyhodí chybu (výchozí 5s).
 * \return MoveResult struktura s úspěšností (success) a skutečně ujetou vzdáleností (traveled_mm).
 */
inline MoveResult move_acc_avoid(float mm, float speed, std::function<bool()> is_obstacle, uint32_t wait_timeout_ms = 5000) {
    if (mm == 0 || speed == 0) { return {true, 0.0f}; }

    Serial.print("move: "); Serial.println(speed);
    // Podpora pro jízdu pozpátku
    bool reverse = (mm < 0);

    // Kalibrace společná: enkodéry naměřily 1973 mm, reálně ujeto 1965 mm → poměr 1965/1973
    float encoder_to_real = 1965.0f / 1973.0f; // Společný přepočet enkodér → reálné mm
    // Individuální kalibrace kol: pravé kolo ujede o 28 mm více než levé na 2000 mm
    // Enkodér pravého kola podměřuje: ukazuje 2000, ale reálně ujelo 2028.
    // Pro P-regulátor musíme enkodér pravého kola škálovat NAHORU: 2028/2000
    float left_calib = 1.0f;                     // Levé kolo je referenční
    float right_calib = 2028.0f / 2000.0f;       // Pravé kolo škálujeme nahoru (enkodér podměřuje)
    float real_target_mm = std::abs(mm); // Cílová vzdálenost v reálných mm (vždy kladná)
    float target_mm = real_target_mm / encoder_to_real; // Odpovídající hodnota na enkodérech
    
    // Normalizace rychlosti (aby znaménko rychlosti odpovídalo směru)
    float abs_target_speed = std::abs(speed);
    float speed_sign = reverse ? -1.0f : 1.0f;
    
    // Vynulování odometrie
    rkMotorsSetPositionLeft(0);
    rkMotorsSetPositionRight(0);
    delay(50); // Krátká pauza, aby se koprocesor stihl zresetovat
    
    float min_speed = 18.0f; // Vráceno na 18. Při nižší rychlosti ztrácí motor kroutící moment a zasekává se
    if (abs_target_speed < min_speed) { abs_target_speed = min_speed; }
    
    // Původní kód: start z min_speed
    // float current_base_speed = min_speed * speed_sign;
    float current_base_speed = 0; // Rozjezd z nulové rychlosti
    
    // Parametry rampy a P-regulátoru
    // Původní kód: pevný accel_step/decel_step = 1
    // float accel_step = 1;
    // float decel_step = 1;
    float accel_step = abs_target_speed / 50.0f; // Rozjezd na max. rychlost zabere cca 50 kroků = 0.5s (při delay 10ms)
    float decel_step = abs_target_speed / 30.0f; // Zpomalení z max. rychlosti zabere cca 30 kroků = 0.3s (rychlejší brzdění)
    float avoid_decel_step = abs_target_speed / 5.0f; // Rychlé zastavení před překážkou (cca 50 ms)
    
    float kp = 1.2f; // P-regulator pro drzeni primeho smeru (zvýšeno z 0.8f pro rychlejší reakci)
    float ki = 0.05f; // Integrální složka pro eliminaci trvalé regulační odchylky
    float diff_sum = 0.0f; // Akumulovaná odchylka (I-složka)
    
    unsigned long start_time = millis();
    unsigned long avoid_wait_start = 0;
    bool waiting_for_obstacle = false;
    
    // Výpočet hrubého timeoutu pro celou cestu
    float speed_mm_per_sec = abs_target_speed * 5.0f; // odhad 100% speed ~ 500 mm/s
    float expected_time_ms = (target_mm / speed_mm_per_sec) * 1000.0f;
    uint32_t general_timeout_ms = (uint32_t)(expected_time_ms * 2.0f + 3000.0f);
    
    // Pomocná funkce pro bezpečné ořezání hodnoty a převod do int8_t
    auto clamp_speed = [](float s) -> int8_t {
        if (s > 100.0f) return 100;
        if (s < -100.0f) return -100;
        return static_cast<int8_t>(s);
    };

    float avg_pos = 0.0f;
    int loop_counter = 0; // Počítadlo průchodů hlavní smyčkou pro zrychlení každý 2. průchod

    while (true) { //hlavnismycka 
        loop_counter++;
        rb::Manager::get().motor(rk::gCtx.motors().idRight()).requestInfo(nullptr);
        float pos_l = rkMotorsGetPositionLeft(true);
        float pos_r = rkMotorsGetPositionRight(false);
        Serial.print(" L ");
        Serial.print(pos_l); Serial.print(" R "); Serial.print(pos_r);
        // RAW hodnoty enkodérů (pro P-regulátor - ten potřebuje skutečný rozdíl otáček)
        float raw_l = std::abs(pos_l);
        float raw_r = std::abs(pos_r);
        // Kalibrované hodnoty (jen pro výpočet průměru a reálné vzdálenosti)
        float calib_l = raw_l * left_calib;
        float calib_r = raw_r * right_calib;
        avg_pos = (calib_l + calib_r) / 2.0f;
        // P-regulátor pracuje s RAW enkodéry (skutečný rozdíl otáček kol)
        float abs_l = raw_l;
        float abs_r = raw_r;
        float abs_curr = std::abs(current_base_speed);

        float real_pos = avg_pos * encoder_to_real; // Přepočet enkodérů na reálnou vzdálenost

        // Porovnáváme v reálných mm (aby robot reálně dojel přesně zadanou vzdálenost)
        if (real_pos >= real_target_mm) {
            real_pos = real_target_mm; // Oříznutí pro přesnost návratové hodnoty
            avg_pos = real_pos / encoder_to_real; // Odpovídající hodnota enkodérů
            break; // Dojeli jsme do cíle
        }
        
        // Časový limit pro celou funkci (pokud se někde nezasekne natrvalo)
        if ((millis() - start_time > general_timeout_ms) && !waiting_for_obstacle) {
            rkMotorsSetSpeed(0, 0);
            return {false, reverse ? -(avg_pos * encoder_to_real) : (avg_pos * encoder_to_real)}; 
        }
        
        float dist_remaining = real_target_mm - real_pos;
        
        bool obstacle = is_obstacle();
        
        if (obstacle) {
            if (!waiting_for_obstacle) {
                // Fáze: Rychle plynule zastavit
                abs_curr -= avoid_decel_step;
                if (abs_curr <= 0) {
                    abs_curr = 0;
                    waiting_for_obstacle = true;
                    avoid_wait_start = millis();
                }
                current_base_speed = abs_curr * speed_sign;
            } else {
                // Fáze: Zastaveno, čekáme na volnou cestu
                current_base_speed = 0;
                if (millis() - avoid_wait_start > wait_timeout_ms) {
                    rkMotorsSetSpeed(0, 0);
                    return {false, reverse ? -(avg_pos * encoder_to_real) : (avg_pos * encoder_to_real)}; // Překážka nezmizela v limitu
                }
            }
        } else {
            if (waiting_for_obstacle) {
                // Překážka právě zmizela, pokračujeme v jízdě
                waiting_for_obstacle = false;
                start_time += (millis() - avoid_wait_start); // Posuneme celkový timeout
                // Původní kód: restart z min_speed
                // current_base_speed = min_speed * speed_sign; 
                current_base_speed = 0; // Rozjezd z nulové rychlosti
            }
            // Dynamická brzdná dráha: čím rychleji AKTUÁLNĚ jede, tím dál před cílem začne brzdit.
            // Přepočítává se každou smyčku podle aktuální rychlosti.
            float required_decel_distance = 4.0f * abs_curr;
            
            if (dist_remaining <= required_decel_distance) {
                // Fáze: Běžné plynulé zpomalování před cílem
                float abs_curr = std::abs(current_base_speed);
                if (loop_counter % 2 == 1) { // ubírá rychlost pro každý druhý průchod smyčkou pro plynulejší brždění
                    abs_curr -= decel_step;
                }
                // Původní kód: brzdění jen do min_speed
                // if (abs_curr < min_speed) abs_curr = min_speed;
                if (abs_curr < 0) abs_curr = 0; // Brzdění až do nulové rychlosti
                current_base_speed = abs_curr * speed_sign;
            } else {
                // Fáze: Zrychlování a konstantní jízda
                float abs_curr = std::abs(current_base_speed);
                if (abs_curr < abs_target_speed) {
                    if (loop_counter % 2 == 1) { // přidává rychlost každý druhý průchod smyčkou pro plynulejší rozjezd 
                        abs_curr += accel_step;
                    }
                    if (abs_curr > abs_target_speed) abs_curr = abs_target_speed;
                }
                current_base_speed = abs_curr * speed_sign;
            }
        }
        
        // Aplikace rychlosti a P-regulátoru pro udržení směru
        if (current_base_speed != 0) {
            float diff = abs_l - abs_r; // Kladné -> levé kolo ujelo víc
            Serial.print(" D "); Serial.print(diff);
            
            // P regulátor        PI regulátor jsem vypnul 
            //diff_sum += diff;
            // Anti-windup limitace (zamezení přetečení integrátoru)
            //diff_sum = std::max(-50.0f, std::min(diff_sum, 50.0f));
            
            // PROBLÉM 3: Couvání s vlečným kolečkem je mechanicky nestabilní. - nepozoruju - zrušeno 
            // Pro couvání použijeme jemnější konstanty (o 40 % menší agresivita).
            // float current_kp = reverse ? (kp * 0.6f) : kp;
            //float current_ki = reverse ? (ki * 0.6f) : ki;
            
            float total_error = (diff * kp);
            //float total_error = (diff * current_kp) + (diff_sum * current_ki);
            
            // Dynamický limit: aby to při malých rychlostech mělo vůbec sílu zatáčet, 
            // garantujeme minimální limit korekce 5%.
            float curr_speed = std::abs(current_base_speed);
            float max_c = std::max(curr_speed * 0.5f, 5.0f); 
            float correction = std::max(-max_c, std::min(total_error, max_c));
                Serial.print(" C "); Serial.print(correction);
                Serial.print(" S "); Serial.print(abs_curr);
            float speed_l_abs = abs_curr - correction; // puvodni ****
            float speed_r_abs = abs_curr + correction;

            // if (diff > 0)
            // float speed_l_abs = abs_curr - correction;
            // float speed_r_abs = abs_curr + correction;
            // else 

            
            // OCHRANA PROTI ZASEKNUTÍ KOLA (Stall prevention)
            // PROBLÉM 1 a 2: Tohle dělalo to prudké škubnutí před cílem a při couvání.
            // Aplikujeme to jen tehdy, když nám do cíle zbývá VÍCE než 15 mm. 
            // Těsně před cílem necháme rychlost volně klesnout i pod min_speed (setrvačnost to dojede).
            // POZOR: Musíme zachovat korekční rozdíl mezi koly, který vytvořil PI regulátor!
            // // Pokud bychom pouze zvedli pomalejší kolo a ubrali rychlejšímu, PI regulátor nemůže účinně korigovat směr.
            // if (dist_remaining > 15.0f) {
            //     if (speed_l_abs < min_speed) {
            //         float boost = min_speed - speed_l_abs;
            //         speed_l_abs += boost;
            //         speed_r_abs += boost; // Stejný boost na obě kola → zachová korekci
            //     }
            //     if (speed_r_abs < min_speed) {
            //         float boost = min_speed - speed_r_abs;
            //         speed_l_abs += boost;
            //         speed_r_abs += boost; // Stejný boost na obě kola → zachová korekci
            //     }
            // }
            auto real_l_speed = clamp_speed(speed_l_abs * speed_sign);
            auto real_r_speed = clamp_speed(speed_r_abs * speed_sign);
                Serial.print(" l "); Serial.print(real_l_speed);
                Serial.print(" r "); Serial.println(real_r_speed);
            rkMotorsSetSpeed(real_l_speed, real_r_speed);
        } else {
            rkMotorsSetSpeed(0, 0);
        }
        delay(10);
    }
    
    rkMotorsSetSpeed(0, 0);
    // Vracíme reálnou ujetou vzdálenost (v reálných mm)
    float real_traveled = avg_pos * encoder_to_real;
    return {true, reverse ? -real_traveled : real_traveled};
}

/**
 * \brief Plynulé otočení na místě DOLEVA s pomalým rozjezdem.
 * 
 * \param angle Úhel vázaný na rotaci (ve stupních).
 * \param speed Maximální rychlost v %.
 * \param roztec_kol Šířka mezi koly (výchozí 155.0 mm).
 * \param korekce Konstanta z kalibrace (výchozí 0.958 pro levou stranu).
 */
inline void TurnOnSpotLeft_acc(float angle, float speed, float roztec_kol = 155.0f, float korekce = 0.958f) {
    if (angle <= 0 || speed <= 0) return;
    
    // Výpočet cílové dráhy v mm pro jedno kolo
    float target_mm = korekce * (M_PI * roztec_kol) * (angle / 360.0f);
    float abs_target_speed = std::abs(speed);
    
    rkMotorsSetPositionLeft(0);
    rkMotorsSetPositionRight(0);
    delay(50); // Reset koprocesoru
    
    float min_speed = 15.0f;
    if (abs_target_speed < min_speed) { abs_target_speed = min_speed; }
    float current_base_speed = min_speed;
    
    // === NASTAVENÍ RAMP ===
    float decel_distance_mm = 4.0f * abs_target_speed; // Delší brzdná dráha (o 60 % delší)
    float accel_step = abs_target_speed / 100.0f; // Jemná akcelerace
    float decel_step = abs_target_speed / 200.0f; // Poloviční zpomalení (klouže déle)
    
    float kp = 0.8f; 
    float max_corr = 8.0f;
    
    unsigned long start_time = millis();
    uint32_t general_timeout_ms = 10000; // Timeout 10 vteřin
    
    auto clamp_speed = [](float s) -> int8_t {
        if (s > 100.0f) return 100;
        if (s < -100.0f) return -100;
        return static_cast<int8_t>(s);
    };

    float avg_pos = 0.0f;

    while (true) {
        rb::Manager::get().motor(rk::gCtx.motors().idRight()).requestInfo(nullptr);
        float pos_l = rkMotorsGetPositionLeft(true);
        float pos_r = rkMotorsGetPositionRight(false);
        float abs_l = std::abs(pos_l);
        float abs_r = std::abs(pos_r);
        avg_pos = (abs_l + abs_r) / 2.0f;
        
        if (avg_pos >= target_mm) break;
        if (millis() - start_time > general_timeout_ms) break;
        
        float dist_remaining = target_mm - avg_pos;
        
        if (dist_remaining <= decel_distance_mm) {
            current_base_speed -= decel_step;
            if (current_base_speed < min_speed) current_base_speed = min_speed;
        } else {
            if (current_base_speed < abs_target_speed) {
                current_base_speed += accel_step;
                if (current_base_speed > abs_target_speed) current_base_speed = abs_target_speed;
            }
        }
        
        // P-regulátor na přesný střed otáčení (obě kola ujedou absolutně stejnou vzdálenost)
        float diff = abs_l - abs_r; // Kladné -> levé ujelo víc
        float correction = std::max(-max_corr, std::min(diff * kp, max_corr));
        
        float speed_l_mag = current_base_speed - correction;
        float speed_r_mag = current_base_speed + correction;
        
        // DOLEVA = levé kolo couvá, pravé jede vpřed
        rkMotorsSetSpeed(clamp_speed(-speed_l_mag), clamp_speed(speed_r_mag));
        delay(10);
    }
    rkMotorsSetSpeed(0, 0);
}

/**
 * \brief Plynulé otočení na místě DOPRAVA s pomalým rozjezdem.
 * 
 * \param angle Úhel vázaný na rotaci (ve stupních).
 * \param speed Maximální rychlost v %.
 * \param roztec_kol Šířka mezi koly (výchozí 155.0 mm).
 * \param korekce Konstanta z kalibrace (výchozí 0.948 pro pravou stranu).
 */
inline void TurnOnSpotRight_acc(float angle, float speed, float roztec_kol = 155.0f, float korekce = 0.948f) {
    if (angle <= 0 || speed <= 0) return;
    
    float target_mm = korekce * (M_PI * roztec_kol) * (angle / 360.0f);
    float abs_target_speed = std::abs(speed);
    
    rkMotorsSetPositionLeft(0);
    rkMotorsSetPositionRight(0);
    delay(50);
    
    float min_speed = 15.0f;
    if (abs_target_speed < min_speed) { abs_target_speed = min_speed; }
    float current_base_speed = min_speed;
    
    // === NASTAVENÍ RAMP ===
    float decel_distance_mm = 4.0f * abs_target_speed; 
    float accel_step = abs_target_speed / 100.0f; 
    float decel_step = abs_target_speed / 200.0f; 
    
    float kp = 0.8f; 
    float max_corr = 8.0f;
    
    unsigned long start_time = millis();
    uint32_t general_timeout_ms = 10000;
    
    auto clamp_speed = [](float s) -> int8_t {
        if (s > 100.0f) return 100;
        if (s < -100.0f) return -100;
        return static_cast<int8_t>(s);
    };

    float avg_pos = 0.0f;

    while (true) {
        rb::Manager::get().motor(rk::gCtx.motors().idRight()).requestInfo(nullptr);
        float pos_l = rkMotorsGetPositionLeft(true);
        float pos_r = rkMotorsGetPositionRight(false);
        float abs_l = std::abs(pos_l);
        float abs_r = std::abs(pos_r);
        avg_pos = (abs_l + abs_r) / 2.0f;
        
        if (avg_pos >= target_mm) break;
        if (millis() - start_time > general_timeout_ms) break;
        
        float dist_remaining = target_mm - avg_pos;
        if (dist_remaining <= decel_distance_mm) {
            current_base_speed -= decel_step;
            if (current_base_speed < min_speed) current_base_speed = min_speed;
        } else {
            if (current_base_speed < abs_target_speed) {
                current_base_speed += accel_step;
                if (current_base_speed > abs_target_speed) current_base_speed = abs_target_speed;
            }
        }
        
        float diff = abs_l - abs_r;
        float correction = std::max(-max_corr, std::min(diff * kp, max_corr));
        
        float speed_l_mag = current_base_speed - correction;
        float speed_r_mag = current_base_speed + correction;
        
        // DOPRAVA = levé kolo jede vpřed, pravé couvá
        rkMotorsSetSpeed(clamp_speed(speed_l_mag), clamp_speed(-speed_r_mag));
        delay(10);
    }
    rkMotorsSetSpeed(0, 0);
}

/**
 * \brief Vrátí vzdálenost z předního ultrazvukového senzoru (U1) v mm.
 */
inline uint32_t uz_predni() {
    return rkUltraMeasure(1);
}

/**
 * \brief Vrátí vzdálenost z předního levého ultrazvukového senzoru (U2) v mm.
 */
inline uint32_t uz_predni_levy() {
    return rkUltraMeasure(2);
}

/**
 * \brief Vrátí vzdálenost z předního pravého ultrazvukového senzoru (U3) v mm.
 */
inline uint32_t uz_predni_pravy() {
    return rkUltraMeasure(3);
}

/**
 * \brief Vrátí vzdálenost ze zadního ultrazvukového senzoru (U4) v mm.
 */
inline uint32_t uz_zadek() {
    return rkUltraMeasure(4);
}

/**
 * \brief Vrátí vzdálenost z laserového senzoru (VL53L0X) na Wire1 v mm.
 *        Vrací -1 v případě chyby měření.
 */
inline int uz_laser() {
    return rk_laser_measure("laser");
}



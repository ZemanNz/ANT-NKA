#include "_librk_motors.h"
#include "RBCX.h"
#include <Arduino.h> // Přidání této hlavičky pro funkce delay a millis
#include <thread> // Přidání této hlavičky pro std::thread
#include <iostream>

namespace rk {

Motors::Motors()
    : m_id_left(rb::MotorId::M1)
    , m_id_right(rb::MotorId::M4) {
}

Motors::~Motors() {
}

void Motors::init(const rkConfig& cfg) {
    m_id_left = (rb::MotorId)(cfg.motor_id_left - 1);
    m_id_right = (rb::MotorId)(cfg.motor_id_right - 1);
    m_polarity_switch_left = cfg.motor_polarity_switch_left;
    m_polarity_switch_right = cfg.motor_polarity_switch_right;

    m_wheel_circumference = M_PI * cfg.motor_wheel_diameter;
    m_wheel_circumference_left = M_PI * cfg.left_wheel_diameter;
    m_wheel_circumference_right = M_PI * cfg.right_wheel_diameter;
    konstanta_radius_vnejsi_kolo = cfg.konstanta_radius_vnejsi_kolo;
    konstanta_radius_vnitrni_kolo = cfg.konstanta_radius_vnitrni_kolo;
    korekce_nedotacivosti_left = cfg.korekce_nedotacivosti_left;
    korekce_nedotacivosti_right = cfg.korekce_nedotacivosti_right;
    m_max_speed = cfg.motor_max_ticks_per_second;
    prevod_motoru = cfg.prevod_motoru;
    roztec_kol = cfg.roztec_kol;

    Button1 = cfg.Button1;
    Button2 = cfg.Button2;
    auto& man
        = rb::Manager::get();

    // Set motor power limits
    man
        .setMotors()
        .pwmMaxPercent(m_id_left, cfg.motor_max_power_pct)
        .pwmMaxPercent(m_id_right, cfg.motor_max_power_pct)
        .set();

    if (rb::Manager::get().coprocFwVersion().number >= 0x010100) {
        const MotorConfig motorConf = {
            .velEpsilon = 3,
            .posEpsilon = 8,
            .maxAccel = cfg.motor_max_acceleration,
        };

        man.motor(m_id_left).setConfig(motorConf);
        man.motor(m_id_right).setConfig(motorConf);
    }
    // WiFi inicializace
    if (cfg.enable_wifi_log && cfg.wifi_ssid && cfg.wifi_password) {
        initWifi(cfg.wifi_ssid, cfg.wifi_password);
    }
}

void Motors::setPower(int8_t left, int8_t right) {
    if (m_polarity_switch_left)
        left = -left;
    if (m_polarity_switch_right)
        right = -right;

    rb::Manager::get()
        .setMotors()
        .power(m_id_left, pctToPower(left))
        .power(m_id_right, pctToPower(right))
        .set();
}

void Motors::setPower(int8_t left, int8_t right, uint8_t pwm_pct_left, uint8_t pwm_pct_right) {
    if (m_polarity_switch_left)
        left = -left;
    if (m_polarity_switch_right)
        right = -right;

    rb::Manager::get()
        .setMotors()
        .pwmMaxPercent(m_id_left, pwm_pct_left)
        .pwmMaxPercent(m_id_right, pwm_pct_right)
        .power(m_id_left, pctToPower(left))
        .power(m_id_right, pctToPower(right))
        .set();
}

void Motors::setPowerById(rb::MotorId id, int8_t power) {
    if ((m_polarity_switch_left && id == m_id_left) || (m_polarity_switch_right && id == m_id_right))
        power = -power;

    rb::Manager::get()
        .setMotors()
        .power(id, pctToPower(power))
        .set();
}

void Motors::setSpeed(int8_t left, int8_t right) {
    if (m_polarity_switch_left)
        left = -left;
    if (m_polarity_switch_right)
        right = -right;

    rb::Manager::get()
        .setMotors()
        .speed(m_id_left, pctToSpeed(left))
        .speed(m_id_right, pctToSpeed(right))
        .set();
}

void Motors::setSpeed(int8_t left, int8_t right, uint8_t pwm_pct_left, uint8_t pwm_pct_right) {
    if (m_polarity_switch_left)
        left = -left;
    if (m_polarity_switch_right)
        right = -right;

    rb::Manager::get()
        .setMotors()
        .pwmMaxPercent(m_id_left, pwm_pct_left)
        .pwmMaxPercent(m_id_right, pwm_pct_right)
        .speed(m_id_left, pctToSpeed(left))
        .speed(m_id_right, pctToSpeed(right))
        .set();
}

void Motors::setSpeedById(rb::MotorId id, int8_t power) {
    if ((m_polarity_switch_left && id == m_id_left) || (m_polarity_switch_right && id == m_id_right))
        power = -power;

    rb::Manager::get()
        .setMotors()
        .speed(id, pctToSpeed(power))
        .set();
}


void Motors::drive(float left, float right, float speed_left, float speed_right, dual_callback_t callback) {

    // 1. Ošetření polarity motorů
    if (m_polarity_switch_left)
        left = -left;

    if (m_polarity_switch_right)
        right = -right;

    // 2. Příprava proměnné pro callback, která bude předána do řídicích funkcí motorů
    rb::Motor::callback_t cb;

    if (callback) {
        std::lock_guard<std::mutex> lock(m_dual_callbacks_mu);
        auto itr = m_dual_callbacks.emplace(m_dual_callbacks.end(), DualCb(std::move(callback)));

        cb = [this, itr](rb::Motor& m) {
            std::unique_lock<std::mutex> lock(m_dual_callbacks_mu);
            if (!itr->completed && ++itr->count >= 1) {
                itr->completed = true;
                dual_callback_t localCb;
                localCb.swap(itr->final_cb);
                m_dual_callbacks.erase(itr);
                lock.unlock();
                // Serial.println("Callback odblokovan motorama.");
                localCb();
            }
        };

        // 3. Timeout – připravíme monitor, který zajistí spuštění callbacku, pokud se neprovede včas
        float expectedTimeLeft = 10000;
        float expectedTimeRight = 10000;
        float maxExpectedTime = std::max(expectedTimeLeft, expectedTimeRight);
        const float timeoutSec = maxExpectedTime * 1.37f;

        std::thread([this, itr, timeoutSec]() {
            unsigned long startTime = millis();
            while ((millis() - startTime) < (timeoutSec * 1000)) {
                {
                    std::lock_guard<std::mutex> lock(m_dual_callbacks_mu);
                    if (itr->completed)
                        return;
                }
                delay(10);
            }

            // Po vypršení timeoutu zkontrolujeme znovu
            std::unique_lock<std::mutex> lock(m_dual_callbacks_mu);
            if (!itr->completed) {
                itr->completed = true;
                dual_callback_t localCb;
                localCb.swap(itr->final_cb);
                m_dual_callbacks.erase(itr);
                lock.unlock();
                // Serial.println("Timeout expiroval, callback odblokovan timeoutem.");
                localCb();
            }
        }).detach();
    }

    // 4. Získání referencí na motory
    auto& ml = rb::Manager::get().motor(m_id_left);
    auto& mr = rb::Manager::get().motor(m_id_right);

    // 5. Načtení informací o levém motoru (není třeba callback)
    ml.requestInfo(nullptr);

    // 6. Načtení informací o pravém motoru a nastavení pohybu motorů
    mr.requestInfo([this, left, right, speed_left, speed_right, cb](rb::Motor& mr) {
        auto& ml = rb::Manager::get().motor(m_id_left);

        rb::Manager::get()
            .setMotors()
            .driveToValue(m_id_left, ml.position() + mmToTicks_left(left), pctToSpeed(speed_left), cb)
            .driveToValue(m_id_right, mr.position() + mmToTicks_right(right), pctToSpeed(speed_right), cb)
            .set();
    });

    // Serial.printf("Leva rychlost: %d, Prava rychlost: %d\n", pctToSpeed(speed_left), pctToSpeed(speed_right));
    // Serial.printf("Leva vzdalenost: %d, Prava vzdalenost: %d\n", mmToTicks(left), mmToTicks(right));
}


void Motors::driveById(rb::MotorId id, float mm, uint8_t speed, std::function<void()> callback) {
    if ((m_polarity_switch_left && id == m_id_left) || (m_polarity_switch_right && id == m_id_right))
        mm = -mm;

    auto& m = rb::Manager::get().motor(id);
    
    m.requestInfo([this, mm, speed, id, callback](rb::Motor& m) {
        int32_t ticks;
        if (id == m_id_left) {
            ticks = mmToTicks_left(mm);
        } else if (id == m_id_right) {
            ticks = mmToTicks_right(mm);
        } else {
            ticks = mmToTicks(mm);
        }

        rb::Manager::get()
            .setMotors()
            .driveToValue(id, m.position() + ticks, pctToSpeed(speed), [this, callback, id](rb::Motor& m) {
                callback();
            })
            .set();
    });
}

float Motors::position(rb::MotorId id) {
    float res;
    if (id == m_id_left) {
        res = ticksToMm_left(rb::Manager::get().motor(id).position());
    } else if (id == m_id_right) {
        res = ticksToMm_right(rb::Manager::get().motor(id).position());
    } else {
        res = ticksToMm(rb::Manager::get().motor(id).position());
    }
    if ((m_polarity_switch_left && id == m_id_left) || (m_polarity_switch_right && id == m_id_right))
        res = -res;
    return res;
}

void Motors::setPosition(rb::MotorId id, float positionMm) {
    int32_t ticks;
    if (id == m_id_left) {
        ticks = mmToTicks_left(positionMm);
    } else if (id == m_id_right) {
        ticks = mmToTicks_right(positionMm);
    } else {
        ticks = mmToTicks(positionMm);
    }
    if ((m_polarity_switch_left && id == m_id_left) || (m_polarity_switch_right && id == m_id_right))
        ticks = -ticks;
    rb::Manager::get().motor(id).setCurrentPosition(ticks);
}

void Motors::joystick(int32_t x, int32_t y) {
    x = scale(x);
    y = scale(y);

    int r = ((y - (x / 1.5f)));
    int l = ((y + (x / 1.5f)));

    r = rb::clamp(r, -100, 100);
    l = rb::clamp(l, -100, 100);

    if (r < 0 && l < 0) {
        std::swap(r, l);
    }
    setSpeed(l, r);
}
/////////////////////////////////////////////////////////////////////////////////////////////////
int Motors::timeout_ms(float mm, float speed){
    return static_cast<int>(295 * mm / abs(speed));
}

int16_t Motors::max_rychlost() {
    auto& man = rb::Manager::get();

    man.motor(m_id_left).setCurrentPosition(0);
    man.motor(m_id_right).setCurrentPosition(0);

    int left_pos = 0;
    int right_pos = 0;

    man.motor(m_id_left).requestInfo([&](rb::Motor& info) {
       left_pos = info.position();
        });
    man.motor(m_id_right).requestInfo([&](rb::Motor& info) {
        right_pos = info.position();
        });

    man.motor(m_id_left).power(pctToPower(100));
    man.motor(m_id_right).power(pctToPower(100));

    int start_time = millis();
    // std::cout << "Starting max speed test..." << std::endl;

    while(abs(left_pos) < 5000 && abs(right_pos) < 5000){
        
        man.motor(m_id_left).requestInfo([&](rb::Motor& info) {
       left_pos = info.position();
        });
        man.motor(m_id_right).requestInfo([&](rb::Motor& info) {
        right_pos = info.position();
        });
        delay(5);
    }

    int stop_time = millis();
    man.motor(m_id_right).power(0);
    man.motor(m_id_left).power(0);
    delay(500);
    int duration = stop_time - start_time;

    // std::cout << "Duration: " << duration << " ms" << std::endl

    // << "Left pos: " << left_pos << ", Right pos: " << right_pos << std::endl;

    int16_t a = 3000;
    int duration_now= 10000;
    man.motor(m_id_left).setCurrentPosition(0);
    man.motor(m_id_right).setCurrentPosition(0);

    left_pos = 0;
    right_pos = 0;

    while(duration_now > duration * 1.07){
        man.motor(m_id_left).speed(a);
        man.motor(m_id_right).speed(a);

        int start_time = millis();
        // std::cout << "start time: " << start_time << " ms at speed " << a << std::endl;

        while(abs(left_pos) < 5000 && abs(right_pos) < 5000){
            
            man.motor(m_id_left).requestInfo([&](rb::Motor& info) {
            left_pos = info.position();
                });
                man.motor(m_id_right).requestInfo([&](rb::Motor& info) {
            right_pos = info.position();
                });
            delay(5);
        }

        int stop_time = millis();
        // std::cout << "stop time: " << stop_time << " ms at speed " << a << std::endl;
        man.motor(m_id_right).power(0);
        man.motor(m_id_left).power(0);
        delay(400);
        man.motor(m_id_left).setCurrentPosition(0);
        man.motor(m_id_right).setCurrentPosition(0);
        left_pos = 0;
        right_pos = 0;

        duration_now = stop_time - start_time;
        // std::cout << "Duration now: " << duration_now << " ms at speed " << a << std::endl;
        a = a + 200;
    }
    return (a - 200);

}

void Motors::forward(float mm, float speed) {
    auto& man = rb::Manager::get();
    
    float m_kp = 0.23f; // Proporcionální konstanta
    float m_min_speed = 20.0f; // Minimální rychlost motorů
    float m_max_correction = 8.5f;
    // Reset pozic
    man.motor(m_id_left).setCurrentPosition(0);
    man.motor(m_id_right).setCurrentPosition(0);

    int target_ticks_left = mmToTicks_left(mm);
    int target_ticks_right = mmToTicks_right(mm);
    float left_pos = 0;
    float right_pos = 0;
    float progres_left = 0.0f;
    float progres_right = 0.0f;
    float rozdil_progres = 0.0f;
    // std::cout << "Target ticks left: " << target_ticks_left << " tick right" << target_ticks_right << std::endl;
    // Základní rychlosti s přihlédnutím k polaritě
    float base_speed_left = m_polarity_switch_left ? -speed : speed;
    float base_speed_right = m_polarity_switch_right ? -speed : speed;
    
    unsigned long start_time = millis();
    int timeoutMs = timeout_ms(mm, speed);
    
    while((target_ticks_left > abs(left_pos) || target_ticks_right > abs(right_pos)) && 
          (millis() - start_time < timeoutMs)) {
        
        // Čtení pozic
        man.motor(m_id_left).requestInfo([&](rb::Motor& info) {
             left_pos = info.position();
          });
        man.motor(m_id_right).requestInfo([&](rb::Motor& info) {
             right_pos = info.position();
          });

        delay(10);

        // std::cout << "Left pos: " << left_pos << ", Right pos: " << right_pos << std::endl;
        //print_wifi("Left pos: " + String(left_pos) + " Right pos: " + String(right_pos) + "\n");
        // P regulátor - pracuje s absolutními hodnotami pozic
        progres_left = (float(abs(left_pos)) / float(target_ticks_left));
        progres_right = (float(abs(right_pos)) / float(target_ticks_right));
        rozdil_progres = progres_left - progres_right;


        float correction = rozdil_progres * m_kp * 1800;
        correction = std::max(-m_max_correction, std::min(correction, m_max_correction));
        // std::cout << "Progres L: " << progres_left << ", Progres R: " << progres_right << ", Diff: " << rozdil_progres << ", Correction: " << correction << std::endl;
        // Výpočet korigovaných rychlostí
        float speed_left = base_speed_left;
        float speed_right = base_speed_right;
        
        // Aplikace korekce podle polarity
        if (correction > 0) {
            // Levý je napřed - zpomalit levý
            if (m_polarity_switch_left) {
                speed_left += correction;  // Přidat k záporné rychlosti = zpomalit
                speed_right += correction;  // Odečíst od kladné rychlosti = zrychlit
            } else {
                speed_left -= correction;  // Odečíst od kladné rychlosti = zpomalit
                speed_right -= correction;  // Přidat ke kladné rychlosti = zrychlit
            }
        } else if (correction < 0) {
            // Pravý je napřed - zpomalit pravý
            if (m_polarity_switch_right) {
                speed_right -= correction;  // Odečíst od záporné rychlosti = zpomalit
                speed_left -= correction;  // Přidat k záporné rychlosti = zrychlit
            } else {
                speed_right += correction;  // Přidat ke kladné rychlosti = zpomalit
                speed_left += correction;  // Odečíst od kladné rychlosti = zrychlit
            }
        }
        
        // Zajištění minimální rychlosti
        if (abs(speed_left) < m_min_speed && abs(speed_left) > 0) {
            speed_left = (speed_left > 0) ? m_min_speed : -m_min_speed;
        }
        if (abs(speed_right) < m_min_speed && abs(speed_right) > 0) {
            speed_right = (speed_right > 0) ? m_min_speed : -m_min_speed;
        }
        
        // Nastavení výkonu motorů
        man.motor(m_id_left).speed(pctToSpeed(speed_left ));
        man.motor(m_id_right).speed(pctToSpeed(speed_right ));
        // std::cout << "Speed left: " << speed_left << ", Speed right: " << speed_right << std::endl;
        //print_wifi("Speed left: " + String(speed_left) + " Speed right: " + String(speed_right) + "\n");
    }
    
    // Zastavení motorů
    man.motor(m_id_left).speed(0);
    man.motor(m_id_right).speed(0);
    man.motor(m_id_left).power(0);
    man.motor(m_id_right).power(0);
}


void Motors::backward(float mm, float speed) {
    auto& man = rb::Manager::get();
    
    float m_kp = 0.23f; // Proporcionální konstanta
    float m_min_speed = 20.0f; // Minimální rychlost motorů
    float m_max_correction = 8.5f;
    // Reset pozic
    man.motor(m_id_left).setCurrentPosition(0);
    man.motor(m_id_right).setCurrentPosition(0);

    int target_ticks_left = mmToTicks_left(mm);
    int target_ticks_right = mmToTicks_right(mm);
    float left_pos = 0;
    float right_pos = 0;
    float progres_left = 0.0f;
    float progres_right = 0.0f;
    float rozdil_progres = 0.0f;
    // std::cout << "Target ticks left: " << target_ticks_left << " tick right" << target_ticks_right << std::endl;
    // Základní rychlosti s přihlédnutím k polaritě
    float base_speed_left = m_polarity_switch_left ? speed : -speed;
    float base_speed_right = m_polarity_switch_right ? speed : -speed;
    
    unsigned long start_time = millis();
    int timeoutMs = timeout_ms(mm, speed);
    
    while((target_ticks_left > abs(left_pos) || target_ticks_right > abs(right_pos)) && 
          (millis() - start_time < timeoutMs)) {
        
        // Čtení pozic
        man.motor(m_id_left).requestInfo([&](rb::Motor& info) {
             left_pos = info.position();
          });
        man.motor(m_id_right).requestInfo([&](rb::Motor& info) {
             right_pos = info.position();
          });

        delay(10);
        // std::cout << "Left pos: " << left_pos << ", Right pos: " << right_pos << std::endl;
        //print_wifi("Left pos: " + String(left_pos) + " Right pos: " + String(right_pos) + "\n");
        // P regulátor - pracuje s absolutními hodnotami pozic
        progres_left = (float(abs(left_pos)) / float(target_ticks_left));
        progres_right = (float(abs(right_pos)) / float(target_ticks_right));
        rozdil_progres = progres_left - progres_right;

        float correction = rozdil_progres * m_kp * 1800;
        correction = std::max(-m_max_correction, std::min(correction, m_max_correction));
        // std::cout << "Progres L: " << progres_left << ", Progres R: " << progres_right << ", Diff: " << rozdil_progres << ", Correction: " << correction << std::endl;
        // Výpočet korigovaných rychlostí
        float speed_left = base_speed_left;
        float speed_right = base_speed_right;
        
        // Aplikace korekce podle polarity
        if (rozdil_progres > 0) {
            // Levý je napřed - zpomalit levý
            if (m_polarity_switch_left) {
                speed_left -= correction;  // Přidat k záporné rychlosti = zpomalit
                speed_right -= correction;  // Odečíst od kladné rychlosti = zrychlit
            } else {
                speed_left += correction;  // Odečíst od kladné rychlosti = zpomalit
                speed_right += correction;  // Přidat ke kladné rychlosti = zrychlit
            }
        } else if (rozdil_progres < 0) {
            // Pravý je napřed - zpomalit pravý
            if (m_polarity_switch_right) {
                speed_right += correction;  // Odečíst od záporné rychlosti = zpomalit
                speed_left += correction;  // Přidat k záporné rychlosti = zrychlit
            } else {
                speed_right -= correction;  // Přidat ke kladné rychlosti = zpomalit
                speed_left -= correction;  // Odečíst od kladné rychlosti = zrychlit
            }
        }
        
        // Zajištění minimální rychlosti
        if (abs(speed_left) < m_min_speed && abs(speed_left) > 0) {
            speed_left = (speed_left > 0) ? m_min_speed : -m_min_speed;
        }
        if (abs(speed_right) < m_min_speed && abs(speed_right) > 0) {
            speed_right = (speed_right > 0) ? m_min_speed : -m_min_speed;
        }
        
        // Nastavení výkonu motorů
        man.motor(m_id_left).speed(pctToSpeed(speed_left ));
        man.motor(m_id_right).speed(pctToSpeed(speed_right ));
        // std::cout << "Speed left: " << speed_left << ", Speed right: " << speed_right << std::endl;
        //print_wifi("Speed left: " + String(speed_left) + " Speed right: " + String(speed_right) + "\n");
    }
    
    // Zastavení motorů
    man.motor(m_id_left).speed(0);
    man.motor(m_id_right).speed(0);
    man.motor(m_id_left).power(0);
    man.motor(m_id_right).power(0);
}




void Motors::turn_on_spot_right(float angle, float speed) {
    auto& man = rb::Manager::get();
    
    // Reset pozic
    man.motor(m_id_left).setCurrentPosition(0);
    man.motor(m_id_right).setCurrentPosition(0);

    int target_ticks = korekce_nedotacivosti_right * mmToTicks((M_PI * roztec_kol) * (angle / 360.0f));
    int left_pos = 0;
    int right_pos = 0;
    // std::cout << "Target ticks: " << target_ticks << std::endl;
    // Základní rychlosti s přihlédnutím k polaritě

    float step = 5.0f; // velikost kroku pro zvyšování rychlosti
    float min_speed = 15.0f; // minimální rychlost
    //float progres = 0.0f;
    int o_kolik_drive_zpomalovat = 1400;
    byte a = 0;
    byte deaccelating = byte(240/abs(speed)); // počet cyklů mezi zpomalováním
    // std::cout << "Deaccelating every " << int(deaccelating) << " cycles." << std::endl;
    float max_correction = 7.0f; // Maximální korekce rychlosti

    float base_speed_left = m_polarity_switch_left ? -speed : speed;
    float base_speed_right = m_polarity_switch_right ? speed : -speed;

    float step_left = (base_speed_left > 0) ? step : -step;
    float step_right = (base_speed_right > 0) ? step : -step;

    float speed_left = base_speed_left;
    float speed_right = base_speed_right;

    float l_speed = 0.0f;
    float r_speed = 0.0f;

    unsigned long start_time = millis();
    const int timeoutMs = 10000; // 10 second timeout
    
    while((target_ticks > (abs(left_pos) +5) || target_ticks > (abs(right_pos) + 5)) && 
          (millis() - start_time < timeoutMs)) {
        man.motor(m_id_left).requestInfo([&](rb::Motor& info) {
             left_pos = info.position();
          });
        man.motor(m_id_right).requestInfo([&](rb::Motor& info) {
             right_pos = info.position();
          });
        delay(10);
        // std::cout << "Left pos: " << left_pos << ", Right pos: " << right_pos << std::endl;
        //progres = (float(abs(left_pos)) + float(abs(right_pos))) / (2.0f * float(target_ticks));
        // Zrychlení
        if (((float(abs(left_pos)) + float(abs(right_pos)))/2) < (target_ticks - o_kolik_drive_zpomalovat)  && (abs(speed_left) < abs(speed) || abs(speed_right) < abs(speed))) {

            if((abs(base_speed_left) < abs(speed))) {
                speed_left += step_left;
            }
            if((abs(base_speed_right) < abs(speed))) {
                speed_right += step_right;
            }
            // std::cout << "Accelerating" << std::endl;
        }
        // Zpomalování
        else if (((float(abs(left_pos)) + float(abs(right_pos))) / 2) > (target_ticks - o_kolik_drive_zpomalovat)) {
            if(a % deaccelating == 0) { // zpomaluj jen každých 8 cyklů pro plynulejší zpomalení
                if (abs(speed_left) > min_speed) {
                    speed_left -= step_left;
                }
                if (abs(speed_right) > min_speed) {
                    speed_right -= step_right;
                }
                a = 0; 
                // std::cout << "Deaccelerating" << std::endl;
            }
            
            a++;
        }
        l_speed = speed_left;
        r_speed = speed_right;
        //Regulace
        float error = abs(left_pos) - abs(right_pos);
        float correction = error * 0.18f; // Proporcionální konstanta
        if(abs(correction) > max_correction) {
            correction = (correction > 0) ? max_correction : -max_correction;
        }

        if (error > 0) {
            // Levý je napřed - zpomalit levý
            if (base_speed_left > 0) {
                l_speed -= correction;  // Odečíst od kladné rychlosti = zpomalit
                r_speed += correction;  // Přidat ke kladné rychlosti = zrychlit
            } else {
                l_speed += correction;  // Přidat k záporné rychlosti = zpomalit
                r_speed -= correction;  // Odečíst od záporné rychlosti = zrychlit
            }
        }
        else {
            // Pravý je napřed - zpomalit pravý
            if (base_speed_right > 0) {
                r_speed += correction;  // Odečíst od záporné rychlosti = zpomalit
                l_speed -= correction;  // Přidat k záporné rychlosti = zrychlit
            } else {
                r_speed -= correction;  // Přidat ke kladné rychlosti = zpomalit
                l_speed += correction;  // Odečíst od kladné rychlosti = zrychlit
            }
        }
        // std::cout << "Error: " << error << ", Correction: " << correction << std::endl;

        //omezeni na minimum
        if (abs(l_speed) < min_speed) {
            l_speed = (l_speed > 0) ? min_speed : -min_speed;
        }
        if (abs(r_speed) < min_speed) {
            r_speed = (r_speed > 0) ? min_speed : -min_speed;
        }
        if(abs(l_speed) > (abs(base_speed_left)+ 10)) {
            if(base_speed_left > 0) l_speed = base_speed_left + 10;
            else l_speed = base_speed_left - 10;
        }
        if(abs(r_speed) > (abs(base_speed_right)+ 10)) {
            if(base_speed_right > 0) r_speed = base_speed_right + 10;
            else r_speed = base_speed_right - 10;
        }
        // Nastavení výkonu motorů
        man.motor(m_id_left).speed(pctToSpeed(l_speed));
        man.motor(m_id_right).speed(pctToSpeed(r_speed));
        // std::cout << "Speed left: " << l_speed << ", Speed right: " << r_speed << std::endl;
        // std::cout << "Progres: " << progres * 100.0f << "%" << std::endl;
        // std::cout << "------------------------" << std::endl;
    }
    man.motor(m_id_left).speed(0);
    man.motor(m_id_right).speed(0);
    man.motor(m_id_left).power(0);
    man.motor(m_id_right).power(0);
}


void Motors::turn_on_spot_left(float angle, float speed) {
    auto& man = rb::Manager::get();
    
    // Reset pozic
    man.motor(m_id_left).setCurrentPosition(0);
    man.motor(m_id_right).setCurrentPosition(0);

    int target_ticks = korekce_nedotacivosti_left * mmToTicks((M_PI * roztec_kol) * (angle / 360.0f));
    int left_pos = 0;
    int right_pos = 0;
    // std::cout << "Target ticks: " << target_ticks << std::endl;
    // Základní rychlosti s přihlédnutím k polaritě

    float step = 5.0f; // velikost kroku pro zvyšování rychlosti
    float min_speed = 15.0f; // minimální rychlost
    //float progres = 0.0f;
    int o_kolik_drive_zpomalovat = 1400;
    byte a = 0;
    byte deaccelating = byte(240/abs(speed)); // počet cyklů mezi zpomalováním
    // std::cout << "Deaccelating every " << int(deaccelating) << " cycles." << std::endl;
    float max_correction = 7.0f; // Maximální korekce rychlosti

    float base_speed_left = m_polarity_switch_left ? speed : -speed;
    float base_speed_right = m_polarity_switch_right ? -speed : speed;

    float step_left = (base_speed_left > 0) ? step : -step;
    float step_right = (base_speed_right > 0) ? step : -step;

    // std::cout << " base_speed_left: " << base_speed_left << ", base_speed_right: " << base_speed_right << std::endl;

    float speed_left = base_speed_left;
    float speed_right = base_speed_right;

    float l_speed = 0.0f;
    float r_speed = 0.0f;

    unsigned long start_time = millis();
    const int timeoutMs = 10000; // 10 second timeout
    
    while((target_ticks > (abs(left_pos) +5) || target_ticks > (abs(right_pos) + 5)) && 
          (millis() - start_time < timeoutMs)) {
        man.motor(m_id_left).requestInfo([&](rb::Motor& info) {
             left_pos = info.position();
          });
        man.motor(m_id_right).requestInfo([&](rb::Motor& info) {
             right_pos = info.position();
          });
        delay(10);
        // std::cout << "Left pos: " << left_pos << ", Right pos: " << right_pos << std::endl;
        //progres = (float(abs(left_pos)) + float(abs(right_pos))) / (2.0f * float(target_ticks));


        // Zrychlení
        if (((float(abs(left_pos)) + float(abs(right_pos)))/2) < (target_ticks - o_kolik_drive_zpomalovat)  && (abs(speed_left) < abs(speed) || abs(speed_right) < abs(speed))) {

            if((abs(base_speed_left) < abs(speed))) {
                speed_left += step_left;
            }
            if((abs(base_speed_right) < abs(speed))) {
                speed_right += step_right;
            }
            // std::cout << "Accelerating" << std::endl;
        }
        // Zpomalování
        else if (((float(abs(left_pos)) + float(abs(right_pos))) / 2) > (target_ticks - o_kolik_drive_zpomalovat)) {
            if(a % deaccelating == 0) { // zpomaluj jen každých 8 cyklů pro plynulejší zpomalení
                if (abs(speed_left) > min_speed) {
                    speed_left -= step_left;
                }
                if (abs(speed_right) > min_speed) {
                    speed_right -= step_right;
                }
                a = 0; 
                // std::cout << "Deaccelerating" << std::endl;
            }
            
            a++;
        }
        l_speed = speed_left;
        r_speed = speed_right;
        // std::cout << " Intermediate l_speed: " <<  l_speed << ", r_speed: " << r_speed << std::endl;
        //Regulace
        float error = abs(left_pos) - abs(right_pos);
        float correction = error * 0.18f; // Proporcionální konstanta
        if(abs(correction) > max_correction) {
            correction = (correction > 0) ? max_correction : -max_correction;
        }

        if (error > 0) {
            // Levý je napřed - zpomalit levý
            if (base_speed_left > 0) {
                l_speed -= correction;  // Odečíst od kladné rychlosti = zpomalit
                r_speed += correction;  // Přidat ke kladné rychlosti = zrychlit
            } else {
                l_speed += correction;  // Přidat k záporné rychlosti = zpomalit
                r_speed -= correction;  // Odečíst od záporné rychlosti = zrychlit
            }
        }
        else {
            // Pravý je napřed - zpomalit pravý
            if (base_speed_right > 0) {
                r_speed += correction;  // Odečíst od záporné rychlosti = zpomalit
                l_speed -= correction;  // Přidat k záporné rychlosti = zrychlit
            } else {
                r_speed -= correction;  // Přidat ke kladné rychlosti = zpomalit
                l_speed += correction;  // Odečíst od kladné rychlosti = zrychlit
            }
        }
        // std::cout << "Error: " << error << ", Correction: " << correction << std::endl;
        // std::cout << " After correction l_speed: " <<  l_speed << ", r_speed: " << r_speed << std::endl;
        //omezeni na minimum
        if (abs(l_speed) < min_speed) {
            l_speed = (l_speed > 0) ? min_speed : -min_speed;
        }
        if (abs(r_speed) < min_speed) {
            r_speed = (r_speed > 0) ? min_speed : -min_speed;
        }
        if(abs(l_speed) > (abs(base_speed_left)+ 10)) {
            if(base_speed_left > 0) l_speed = base_speed_left + 10;
            else l_speed = base_speed_left - 10;
        }
        if(abs(r_speed) > (abs(base_speed_right)+ 10)) {
            if(base_speed_right > 0) r_speed = base_speed_right + 10;
            else r_speed = base_speed_right - 10;
        }
        // Nastavení výkonu motorů
        man.motor(m_id_left).speed(pctToSpeed(l_speed));
        man.motor(m_id_right).speed(pctToSpeed(r_speed));
        // std::cout << "Speed left: " << l_speed << ", Speed right: " << r_speed << std::endl;
        // std::cout << "Progres: " << progres * 100.0f << "%" << std::endl;
        // std::cout << "------------------------" << std::endl;
    }
    man.motor(m_id_left).speed(0);
    man.motor(m_id_right).speed(0);
    man.motor(m_id_left).power(0);
    man.motor(m_id_right).power(0);
}


void Motors::radius_right(float radius, float angle, float speed) {
    auto& man = rb::Manager::get();
    
    // Reset pozic
    man.motor(m_id_left).setCurrentPosition(0);
    man.motor(m_id_right).setCurrentPosition(0);

    // Výpočet drah pro zatáčku VPRAVO
    float distance_left = (((radius + roztec_kol) * PI * angle) / 180) * konstanta_radius_vnejsi_kolo;  // vnější kolo
    float distance_right = (( radius * PI * angle) / 180)* konstanta_radius_vnitrni_kolo;               // vnitřní kolo

    int target_ticks_left = mmToTicks_left(distance_left);
    int target_ticks_right = mmToTicks_right(distance_right);

    // Základní výpočet rychlostí
    float target_speed_left = speed;  // vnější kolo
    float target_speed_right = speed * (radius / (roztec_kol + radius));  // vnitřní kolo
    
    // std::cout << "-----------------------------" << std::endl;
    // std::cout << "Target ticks:   L=" << target_ticks_left << "  R=" << target_ticks_right << std::endl;
    // std::cout << "Target speeds:  L=" << target_speed_left << "  R=" << target_speed_right << std::endl;

    // Úprava polarity
    if (m_polarity_switch_left)
        target_speed_left = -target_speed_left;
    if (m_polarity_switch_right)
        target_speed_right = -target_speed_right;

    // Proměnné pro zrychlení a zpomalení - RŮZNÉ PRO VNĚJŠÍ A VNITŘNÍ KOLO
    float step_outer = 3.0f; // větší krok pro vnější kolo
    float step_inner = 1.5f; // menší krok pro vnitřní kolo (menší rychlost)
    
    // MINIMÁLNÍ RYCHLOSTI V POMĚRU - místo pevné hodnoty
    float base_min_speed = 15.0f; // základní minimální rychlost
    float min_speed_outer = base_min_speed; // vnější kolo - vyšší minimum
    float min_speed_inner = base_min_speed * (target_speed_right / target_speed_left); // vnitřní kolo - poměrně nižší minimum
    
    // Omezení minimálních rychlostí na rozumné meze
    min_speed_outer = std::max(12.0f, std::min(min_speed_outer, 20.0f));
    min_speed_inner = std::max(10.0f, std::min(min_speed_inner, 18.0f));
    
    // std::cout << "Min speeds:     L=" << min_speed_outer << "  R=" << min_speed_inner << std::endl;
    
    // RŮZNÉ BODY ZAČÁTKU ZPOMALOVÁNÍ - vnitřní kolo začne zpomalovat později
    int zpomalovat_outer = 1400; // vnější kolo začne zpomalovat dříve
    int zpomalovat_inner = zpomalovat_outer / 2; // vnitřní kolo začne zpomalovat 2x později
    
    byte counter_outer = 0;
    byte counter_inner = 0;
    
    // RŮZNÁ FREKVENCE ZPOMALOVÁNÍ
    byte deaccelating_outer = byte(240/abs(speed)); // častější zpomalování pro vnější kolo
    byte deaccelating_inner = byte(480/abs(speed)); // řidší zpomalování pro vnitřní kolo (2x méně často)

    // std::cout << "Deaccelating outer every " << int(deaccelating_outer) << " cycles." << std::endl;
    // std::cout << "Deaccelating inner every " << int(deaccelating_inner) << " cycles." << std::endl;

    // Směr kroku
    float step_dir_outer = (target_speed_left > 0) ? step_outer : -step_outer;
    float step_dir_inner = (target_speed_right > 0) ? step_inner : -step_inner;

    float current_speed_left = 0;
    float current_speed_right = 0;

    // P regulátor - konstanty
    const float Kp = 1.47f;
    const float max_speed_adjust = 5.9f;        


    int timeoutMs = 10000;
    unsigned long start_time = millis();
    bool left_done = false;
    bool right_done = false;

    int left_pos = 0;
    int right_pos = 0;

    while(millis() - start_time < timeoutMs) {
        // Synchronní čtení pozic
        man.motor(m_id_left).requestInfo([&](rb::Motor& info) {
             left_pos = info.position();
          });
        man.motor(m_id_right).requestInfo([&](rb::Motor& info) {
             right_pos = info.position();
          });

        delay(10);
        
        // VÝPOČET CELKOVÉHO POKROKU PRO KAŽDÉ KOLO ZVLÁŠŤ
        float progress_left = (float)abs(left_pos) / abs(target_ticks_left);
        float progress_right = (float)abs(right_pos) / abs(target_ticks_right);
        
        // ZRYCHLENÍ - PRO OBĚ KOLA
        if (abs(left_pos) < (abs(target_ticks_left) - zpomalovat_outer) && 
            abs(current_speed_left) < abs(target_speed_left)) {
            current_speed_left += step_dir_outer;
            // std::cout << "Zrychlování vnější kolo" << std::endl;
        }
        
        if (abs(right_pos) < (abs(target_ticks_right) - zpomalovat_inner) && 
            abs(current_speed_right) < abs(target_speed_right)) {
            current_speed_right += step_dir_inner;
            // std::cout << "Zrychlování vnitřní kolo" << std::endl;
        }
        
        // ZPOMALOVÁNÍ - VNĚJŠÍ KOLO (začne dříve a častěji)
        if (abs(left_pos) >= (abs(target_ticks_left) - zpomalovat_outer)) {
            if(counter_outer % deaccelating_outer == 0) {
                if (abs(current_speed_left) > min_speed_outer) {
                    current_speed_left -= step_dir_outer;
                }
                counter_outer = 0;
                // std::cout << "Zpomalování vnější kolo" << std::endl;
            }
            counter_outer++;
        }
        
        // ZPOMALOVÁNÍ - VNITŘNÍ KOLO (začne později a řidčeji)
        if (abs(right_pos) >= (abs(target_ticks_right) - zpomalovat_inner)) {
            if(counter_inner % deaccelating_inner == 0) {
                if (abs(current_speed_right) > min_speed_inner) {
                    current_speed_right -= step_dir_inner;
                }
                counter_inner = 0;
                // std::cout << "Zpomalování vnitřní kolo" << std::endl;
            }
            counter_inner++;
        }

        // Výpis informací o progresu, pozicích a rychlostech
        // std::cout << "Vnější kolo: " << left_pos << "/" << target_ticks_left 
        //           << " (" << progress_left * 100 << "%), Rychlost: " << current_speed_left << " | "
        //           << "Vnitřní kolo: " << right_pos << "/" << target_ticks_right 
        //           << " (" << progress_right * 100 << "%), Rychlost: " << current_speed_right << std::endl;
        
        // P REGULÁTOR - synchronizace kol
        float progress_error = progress_left - progress_right;
        float speed_adjust = Kp * progress_error;
        
        // Omezení úpravy rychlosti
        if (speed_adjust > max_speed_adjust) speed_adjust = max_speed_adjust;
        if (speed_adjust < -max_speed_adjust) speed_adjust = -max_speed_adjust;
        
        // Upravené rychlosti
        float adjusted_speed_left = current_speed_left;
        float adjusted_speed_right = current_speed_right;
        
        if (!left_done && !right_done) {
            // Pokud vnější kolo (levé) je napřed, zpomalíme ho a/nebo zrychlíme vnitřní
            if (progress_error > 0) {
                adjusted_speed_left = current_speed_left * (1.0f - abs(speed_adjust));
                adjusted_speed_right = current_speed_right * (1.0f + abs(speed_adjust));
            }
            // Pokud vnitřní kolo (pravé) je napřed, zpomalíme ho a/nebo zrychlíme vnější
            else if (progress_error < 0) {
                adjusted_speed_left = current_speed_left * (1.0f + abs(speed_adjust));
                adjusted_speed_right = current_speed_right * (1.0f - abs(speed_adjust));
            }
            
            // Zajištění minimální rychlosti - S RŮZNÝMI MINIMY PRO KAŽDÉ KOLO
            if (abs(adjusted_speed_left) < min_speed_outer && abs(adjusted_speed_left) > 0) {
                adjusted_speed_left = (adjusted_speed_left > 0) ? min_speed_outer : -min_speed_outer;
            }
            if (abs(adjusted_speed_right) < min_speed_inner && abs(adjusted_speed_right) > 0) {
                adjusted_speed_right = (adjusted_speed_right > 0) ? min_speed_inner : -min_speed_inner;
            }
            
            // Aplikace upravených rychlostí
            man.motor(m_id_left).speed(pctToSpeed(adjusted_speed_left));
            man.motor(m_id_right).speed(pctToSpeed(adjusted_speed_right));
        }
        
        // Kontrola dokončení
        if (abs(left_pos)  >= abs(target_ticks_left) && !left_done) {
            left_done = true;
            man.motor(m_id_left).speed(0);
            man.motor(m_id_left).power(0);
            // std::cout << "Vnější kolo dokončeno" << std::endl;
        }
        
        if (abs(right_pos) >= abs(target_ticks_right) && !right_done) {
            right_done = true;
            man.motor(m_id_right).speed(0);
            man.motor(m_id_right).power(0);
            // std::cout << "Vnitřní kolo dokončeno" << std::endl;
        }
        
        if (left_done && right_done) {
            break;
        }
    }
    
    // Zastavení motorů
    man.motor(m_id_left).speed(0);
    man.motor(m_id_right).speed(0);
    man.motor(m_id_left).power(0);
    man.motor(m_id_right).power(0);
    
    // std::cout << "Radius right completed!" << std::endl;
}

void Motors::radius_left(float radius, float angle, float speed) {
    auto& man = rb::Manager::get();
    
    // Reset pozic
    man.motor(m_id_left).setCurrentPosition(0);
    man.motor(m_id_right).setCurrentPosition(0);

    // Výpočet drah pro zatáčku VLEVO
    float distance_left = ((radius * PI * angle) / 180) * konstanta_radius_vnitrni_kolo;               // vnitřní kolo
    float distance_right = (((radius + roztec_kol) * PI * angle) / 180) * konstanta_radius_vnejsi_kolo;  // vnější kolo

    int target_ticks_left = mmToTicks_left(distance_left);
    int target_ticks_right = mmToTicks_right(distance_right);

    // Základní výpočet rychlostí PRO ZATÁČKU VLEVO
    float target_speed_left = speed * (radius / (roztec_kol + radius));  // vnitřní kolo
    float target_speed_right = speed;  // vnější kolo
    
    // std::cout << "-----------------------------" << std::endl;
    // std::cout << "Target ticks:   L=" << target_ticks_left << "  R=" << target_ticks_right << std::endl;
    // std::cout << "Target speeds:  L=" << target_speed_left << "  R=" << target_speed_right << std::endl;

    // Úprava polarity
    if (m_polarity_switch_left)
        target_speed_left = -target_speed_left;
    if (m_polarity_switch_right)
        target_speed_right = -target_speed_right;

    // Proměnné pro zrychlení a zpomalení - RŮZNÉ PRO VNĚJŠÍ A VNITŘNÍ KOLO
    float step_outer = 3.0f; // větší krok pro vnější kolo (nyní pravé)
    float step_inner = 1.5f; // menší krok pro vnitřní kolo (nyní levé)
    
    // MINIMÁLNÍ RYCHLOSTI V POMĚRU - místo pevné hodnoty
    float base_min_speed = 15.0f; // základní minimální rychlost
    float min_speed_outer = base_min_speed; // vnější kolo - vyšší minimum
    float min_speed_inner = base_min_speed * (target_speed_left / target_speed_right); // vnitřní kolo - poměrně nižší minimum
    
    // Omezení minimálních rychlostí na rozumné meze
    min_speed_outer = std::max(12.0f, std::min(min_speed_outer, 20.0f));
    min_speed_inner = std::max(10.0f, std::min(min_speed_inner, 18.0f));
    
    // std::cout << "Min speeds:     L=" << min_speed_inner << "  R=" << min_speed_outer << std::endl;
    
    // RŮZNÉ BODY ZAČÁTKU ZPOMALOVÁNÍ - vnitřní kolo začne zpomalovat později
    int zpomalovat_outer = 1400; // vnější kolo začne zpomalovat dříve
    int zpomalovat_inner = zpomalovat_outer / 2; // vnitřní kolo začne zpomalovat 2x později
    
    byte counter_outer = 0;
    byte counter_inner = 0;
    
    // RŮZNÁ FREKVENCE ZPOMALOVÁNÍ
    byte deaccelating_outer = byte(240/abs(speed)); // častější zpomalování pro vnější kolo
    byte deaccelating_inner = byte(480/abs(speed)); // řidší zpomalování pro vnitřní kolo (2x méně často)

    // std::cout << "Deaccelating outer every " << int(deaccelating_outer) << " cycles." << std::endl;
    // std::cout << "Deaccelating inner every " << int(deaccelating_inner) << " cycles." << std::endl;

    // Směr kroku
    float step_dir_outer = (target_speed_right > 0) ? step_outer : -step_outer; // vnější kolo je pravé
    float step_dir_inner = (target_speed_left > 0) ? step_inner : -step_inner;  // vnitřní kolo je levé

    float current_speed_left = 0;  // vnitřní kolo
    float current_speed_right = 0; // vnější kolo

    // P regulátor - konstanty
    const float Kp = 1.47f;
    const float max_speed_adjust = 5.9f;

    int timeoutMs = 10000;
    unsigned long start_time = millis();
    bool left_done = false;
    bool right_done = false;

    int left_pos = 0;
    int right_pos = 0;

    while(millis() - start_time < timeoutMs) {
        // Synchronní čtení pozic
        man.motor(m_id_left).requestInfo([&](rb::Motor& info) {
             left_pos = info.position();
          });
        man.motor(m_id_right).requestInfo([&](rb::Motor& info) {
             right_pos = info.position();
          });

        delay(10);
        
        // VÝPOČET CELKOVÉHO POKROKU PRO KAŽDÉ KOLO ZVLÁŠŤ
        float progress_left = (float)abs(left_pos) / abs(target_ticks_left);
        float progress_right = (float)abs(right_pos) / abs(target_ticks_right);
        
        // ZRYCHLENÍ - PRO OBĚ KOLA
        // Vnější kolo (pravé) zrychluje
        if (abs(right_pos) < (abs(target_ticks_right) - zpomalovat_outer) && 
            abs(current_speed_right) < abs(target_speed_right)) {
            current_speed_right += step_dir_outer;
            // std::cout << "Zrychlování vnější kolo" << std::endl;
        }
        
        // Vnitřní kolo (levé) zrychluje
        if (abs(left_pos) < (abs(target_ticks_left) - zpomalovat_inner) && 
            abs(current_speed_left) < abs(target_speed_left)) {
            current_speed_left += step_dir_inner;
            // std::cout << "Zrychlování vnitřní kolo" << std::endl;
        }
        
        // ZPOMALOVÁNÍ - VNĚJŠÍ KOLO (pravé, začne dříve a častěji)
        if (abs(right_pos) >= (abs(target_ticks_right) - zpomalovat_outer)) {
            if(counter_outer % deaccelating_outer == 0) {
                if (abs(current_speed_right) > min_speed_outer) {
                    current_speed_right -= step_dir_outer;
                }
                counter_outer = 0;
                // std::cout << "Zpomalování vnější kolo" << std::endl;
            }
            counter_outer++;
        }
        
        // ZPOMALOVÁNÍ - VNITŘNÍ KOLO (levé, začne později a řidčeji)
        if (abs(left_pos) >= (abs(target_ticks_left) - zpomalovat_inner)) {
            if(counter_inner % deaccelating_inner == 0) {
                if (abs(current_speed_left) > min_speed_inner) {
                    current_speed_left -= step_dir_inner;
                }
                counter_inner = 0;
                // std::cout << "Zpomalování vnitřní kolo" << std::endl;
            }
            counter_inner++;
        }

        // Výpis informací o progresu, pozicích a rychlostech
        // std::cout << "Vnitřní kolo: " << left_pos << "/" << target_ticks_left 
        //           << " (" << progress_left * 100 << "%), Rychlost: " << current_speed_left << " | "
        //           << "Vnější kolo: " << right_pos << "/" << target_ticks_right 
        //           << " (" << progress_right * 100 << "%), Rychlost: " << current_speed_right << std::endl;
        
        // P REGULÁTOR - synchronizace kol
        float progress_error = progress_left - progress_right;
        float speed_adjust = Kp * progress_error;
        
        // Omezení úpravy rychlosti
        if (speed_adjust > max_speed_adjust) speed_adjust = max_speed_adjust;
        if (speed_adjust < -max_speed_adjust) speed_adjust = -max_speed_adjust;
        
        // Upravené rychlosti
        float adjusted_speed_left = current_speed_left;
        float adjusted_speed_right = current_speed_right;
        
        if (!left_done && !right_done) {
            // Pokud vnitřní kolo (levé) je napřed, zpomalíme ho a/nebo zrychlíme vnější
            if (progress_error > 0) {
                adjusted_speed_left = current_speed_left * (1.0f - abs(speed_adjust));
                adjusted_speed_right = current_speed_right * (1.0f + abs(speed_adjust));
            }
            // Pokud vnější kolo (pravé) je napřed, zpomalíme ho a/nebo zrychlíme vnitřní
            else if (progress_error < 0) {
                adjusted_speed_left = current_speed_left * (1.0f + abs(speed_adjust));
                adjusted_speed_right = current_speed_right * (1.0f - abs(speed_adjust));
            }
            
            // Zajištění minimální rychlosti - S RŮZNÝMI MINIMY PRO KAŽDÉ KOLO
            if (abs(adjusted_speed_left) < min_speed_inner && abs(adjusted_speed_left) > 0) {
                adjusted_speed_left = (adjusted_speed_left > 0) ? min_speed_inner : -min_speed_inner;
            }
            if (abs(adjusted_speed_right) < min_speed_outer && abs(adjusted_speed_right) > 0) {
                adjusted_speed_right = (adjusted_speed_right > 0) ? min_speed_outer : -min_speed_outer;
            }
            
            // Aplikace upravených rychlostí
            man.motor(m_id_left).speed(pctToSpeed(adjusted_speed_left));
            man.motor(m_id_right).speed(pctToSpeed(adjusted_speed_right));
        }
        
        // Kontrola dokončení
        if (abs(left_pos)  >= abs(target_ticks_left) && !left_done) {
            left_done = true;
            man.motor(m_id_left).speed(0);
            man.motor(m_id_left).power(0);
            // std::cout << "Vnitřní kolo dokončeno" << std::endl;
        }
        
        if (abs(right_pos) >= abs(target_ticks_right) && !right_done) {
            right_done = true;
            man.motor(m_id_right).speed(0);
            man.motor(m_id_right).power(0);
            // std::cout << "Vnější kolo dokončeno" << std::endl;
        }
        
        if (left_done && right_done) {
            break;
        }
    }
    
    // Zastavení motorů
    man.motor(m_id_left).speed(0);
    man.motor(m_id_right).speed(0);
    man.motor(m_id_left).power(0);
    man.motor(m_id_right).power(0);
    
    // std::cout << "Radius left completed!" << std::endl;
}

void Motors::forward_acc(float mm, float speed) {
    auto& man = rb::Manager::get();
    
    float m_kp = 0.23f; // Proporcionální konstanta
    float m_min_speed = 18.0f; // Minimální rychlost motorů
    float m_max_correction = 5.5f;

    byte step= 3;
    byte deaccelating = byte(250/abs(speed));
    byte a = 1;

    byte b = 1;
    byte accelerating = byte(300/abs(speed));

    // Reset pozic
    man.motor(m_id_left).setCurrentPosition(0);
    man.motor(m_id_right).setCurrentPosition(0);

    int target_ticks_left = mmToTicks_left(mm);
    int target_ticks_right = mmToTicks_right(mm);
    float left_pos = 0;
    float right_pos = 0;
    float progres_left = 0.0f;
    float progres_right = 0.0f;
    float rozdil_progres = 0.0f;
    
    // std::cout << "Target ticks left: " << target_ticks_left << " tick right" << target_ticks_right << std::endl;
    
    // Základní rychlosti s přihlédnutím k polaritě
    float base_speed_left = m_polarity_switch_left ? -speed : speed;
    float base_speed_right = m_polarity_switch_right ? -speed : speed;
    
    float step_left = (base_speed_left > 0) ? step : -step;
    float step_right = (base_speed_right > 0) ? step : -step;
    // Proměnné pro akceleraci
    float current_speed_left = (base_speed_left > 0) ? m_min_speed : -m_min_speed;
    float current_speed_right = (base_speed_right > 0) ? m_min_speed : -m_min_speed;
    int o_kolik_drive_zpomalovat = int(60 * speed);    
    unsigned long start_time = millis();
    int timeoutMs = 1.7 * timeout_ms(mm, speed * 0.8);

    if(timeoutMs < 5000){
        timeoutMs = 5000;
    }
    
    while((target_ticks_left > abs(left_pos) || target_ticks_right > abs(right_pos)) && 
          (millis() - start_time < timeoutMs)) {
        
        // Čtení pozic
        man.motor(m_id_left).requestInfo([&](rb::Motor& info) {
             left_pos = info.position();
          });
        man.motor(m_id_right).requestInfo([&](rb::Motor& info) {
             right_pos = info.position();
          });

        delay(10);

        // std::cout << "Left pos: " << left_pos << ", Right pos: " << right_pos << std::endl;
        
        // Výpočet progresu
        progres_left = (float(abs(left_pos)) / float(target_ticks_left));
        progres_right = (float(abs(right_pos)) / float(target_ticks_right));
        rozdil_progres = progres_left - progres_right;

        // AKCELERACE A DEACELERACE
        float avg_progress = (progres_left + progres_right) / 2.0f;

        if((abs(left_pos) > (target_ticks_left - o_kolik_drive_zpomalovat)) && (abs(right_pos) > (target_ticks_right - o_kolik_drive_zpomalovat))) {
            // FÁZE ZPOMALENÍ
            if(a % deaccelating == 0) { // zpomaluj jen každých 8 cyklů pro plynulejší zpomalení
                if (abs(current_speed_left) > m_min_speed) {
                    current_speed_left -= step_left;
                }
                if (abs(current_speed_right) > m_min_speed) {
                    current_speed_right -= step_right;
                }
                a = 0; 
                // std::cout << "Deaccelerating" << std::endl;
            }
            a++;
            // std::cout << "⬇ZPOMALENÍ" << std::endl;
        }
        // Zrychlení
        else if((abs(current_speed_left) < abs(speed) || abs(current_speed_right) < abs(speed)) && (avg_progress < 0.4)) {
            if(b % accelerating == 0){
                if((abs(current_speed_left) < abs(speed))) {
                    current_speed_left += step_left;
                }
                if((abs(current_speed_right) < abs(speed))) {
                    current_speed_right += step_right;
                }
                // std::cout << "Accelerating" << std::endl;
                b = 0;
            }
            b++;
        }
        else{
            // FÁZE KONSTANTNÍ RYCHLOSTI
            current_speed_left = base_speed_left;
            current_speed_right = base_speed_right;
            // std::cout << "⚡ KONSTANTNÍ" << std::endl;
        }

        float correction = rozdil_progres * m_kp * 1800;
        correction = std::max(-m_max_correction, std::min(correction, m_max_correction));
        // std::cout << "Progres L: " << progres_left << ", Progres R: " << progres_right << ", Diff: " << rozdil_progres << ", Correction: " << correction << std::endl;
        
        // Výpočet korigovaných rychlostí
        float speed_left = current_speed_left;
        float speed_right = current_speed_right;
        
        // Aplikace korekce podle polarity - STEJNÉ JAKO V forward()
        if (correction > 0) {
            // Levý je napřed - zpomalit levý
            if (m_polarity_switch_left) {
                speed_left += correction;
                speed_right += correction;
            } else {
                speed_left -= correction;
                speed_right -= correction;
            }
        } else if (correction < 0) {
            // Pravý je napřed - zpomalit pravý
            if (m_polarity_switch_right) {
                speed_right -= correction;
                speed_left -= correction;
            } else {
                speed_right += correction;
                speed_left += correction;
            }
        }
        
        // Zajištění minimální rychlosti
        if (abs(speed_left) < m_min_speed && abs(speed_left) > 0) {
            speed_left = (speed_left > 0) ? m_min_speed : -m_min_speed;
        }
        if (abs(speed_right) < m_min_speed && abs(speed_right) > 0) {
            speed_right = (speed_right > 0) ? m_min_speed : -m_min_speed;
        }
        
        // Nastavení výkonu motorů
        man.motor(m_id_left).speed(pctToSpeed(speed_left));
        man.motor(m_id_right).speed(pctToSpeed(speed_right));
        
        // std::cout << "Speed left: " << speed_left << ", Speed right: " << speed_right << std::endl;
        // std::cout << "Progress: " << (avg_progress * 100.0f) << "%" << std::endl;
        // std::cout << "----------------------------------------" << std::endl;
    }
    
    // Zastavení motorů
    man.motor(m_id_left).speed(0);
    man.motor(m_id_right).speed(0);
    man.motor(m_id_left).power(0);
    man.motor(m_id_right).power(0);
    
    // std::cout << "forward_acc UKONČENO" << std::endl;
}

void Motors::backward_acc(float mm, float speed) {
    auto& man = rb::Manager::get();
    
    float m_kp = 0.23f; // Proporcionální konstanta
    float m_min_speed = 18.0f; // Minimální rychlost motorů
    float m_max_correction = 5.5f;

    byte step= 3;
    byte deaccelating = byte(250/abs(speed));
    byte a = 1;

    byte b = 1;
    byte accelerating = byte(300/abs(speed));

    // Reset pozic
    man.motor(m_id_left).setCurrentPosition(0);
    man.motor(m_id_right).setCurrentPosition(0);

    int target_ticks_left = mmToTicks_left(mm);
    int target_ticks_right = mmToTicks_right(mm);
    float left_pos = 0;
    float right_pos = 0;
    float progres_left = 0.0f;
    float progres_right = 0.0f;
    float rozdil_progres = 0.0f;
    
    // std::cout << "Target ticks left: " << target_ticks_left << " tick right" << target_ticks_right << std::endl;
    
    // Základní rychlosti s přihlédnutím k polaritě
    float base_speed_left = m_polarity_switch_left ? speed : -speed;
    float base_speed_right = m_polarity_switch_right ? speed : -speed;
    
    float step_left = (base_speed_left > 0) ? step : -step;
    float step_right = (base_speed_right > 0) ? step : -step;

    float current_speed_left = (base_speed_left > 0) ? m_min_speed : -m_min_speed;
    float current_speed_right = (base_speed_right > 0) ? m_min_speed : -m_min_speed;
    // Proměnné pro akceleraci
    int o_kolik_drive_zpomalovat = int(60 * speed);    
    unsigned long start_time = millis();
    int timeoutMs = 1.7 * timeout_ms(mm, speed * 0.8);

    if(timeoutMs < 5000){
        timeoutMs = 5000;
    }
    
    while((target_ticks_left > abs(left_pos) || target_ticks_right > abs(right_pos)) && 
          (millis() - start_time < timeoutMs)) {
        
        // Čtení pozic
        man.motor(m_id_left).requestInfo([&](rb::Motor& info) {
             left_pos = info.position();
          });
        man.motor(m_id_right).requestInfo([&](rb::Motor& info) {
             right_pos = info.position();
          });

        delay(10);

        // std::cout << "Left pos: " << left_pos << ", Right pos: " << right_pos << std::endl;
        
        // Výpočet progresu
        progres_left = (float(abs(left_pos)) / float(target_ticks_left));
        progres_right = (float(abs(right_pos)) / float(target_ticks_right));
        rozdil_progres = progres_left - progres_right;

        // AKCELERACE A DEACELERACE
        float avg_progress = (progres_left + progres_right) / 2.0f;

        if((abs(left_pos) > (target_ticks_left - o_kolik_drive_zpomalovat)) && (abs(right_pos) > (target_ticks_right - o_kolik_drive_zpomalovat))) {
            // FÁZE ZPOMALENÍ
            if(a % deaccelating == 0) { // zpomaluj jen každých 8 cyklů pro plynulejší zpomalení
                if (abs(current_speed_left) > m_min_speed) {
                    current_speed_left -= step_left;
                }
                if (abs(current_speed_right) > m_min_speed) {
                    current_speed_right -= step_right;
                }
                a = 0; 
                // std::cout << "Deaccelerating" << std::endl;
            }
            a++;
            // std::cout << "⬇ZPOMALENÍ" << std::endl;
        }
        // Zrychlení
        else if((abs(current_speed_left) < abs(speed) || abs(current_speed_right) < abs(speed)) && (avg_progress < 0.4)) {
            if(b % accelerating == 0){
                if((abs(current_speed_left) < abs(speed))) {
                    current_speed_left += step_left;
                }
                if((abs(current_speed_right) < abs(speed))) {
                    current_speed_right += step_right;
                }
                // std::cout << "Accelerating" << std::endl;
                b = 0;
            }
            b++;
        }
        else{
            // FÁZE KONSTANTNÍ RYCHLOSTI
            current_speed_left = base_speed_left;
            current_speed_right = base_speed_right;
            // std::cout << "⚡ KONSTANTNÍ" << std::endl;
        }

        float correction = rozdil_progres * m_kp * 1800;
        correction = std::max(-m_max_correction, std::min(correction, m_max_correction));
        // std::cout << "Progres L: " << progres_left << ", Progres R: " << progres_right << ", Diff: " << rozdil_progres << ", Correction: " << correction << std::endl;
        
        // Výpočet korigovaných rychlostí
        float speed_left = current_speed_left;
        float speed_right = current_speed_right;
        
        // Aplikace korekce podle polarity - STEJNÉ JAKO V forward()
        if (correction > 0) {
            // Levý je napřed - zpomalit levý
            if (m_polarity_switch_left) {
                speed_left -= correction;
                speed_right -= correction;
            } else {
                speed_left += correction;
                speed_right += correction;
            }
        } else if (correction < 0) {
            // Pravý je napřed - zpomalit pravý
            if (m_polarity_switch_right) {
                speed_right += correction;
                speed_left += correction;
            } else {
                speed_right -= correction;
                speed_left -= correction;
            }
        }
        
        // Zajištění minimální rychlosti
        if (abs(speed_left) < m_min_speed && abs(speed_left) > 0) {
            speed_left = (speed_left > 0) ? m_min_speed : -m_min_speed;
        }
        if (abs(speed_right) < m_min_speed && abs(speed_right) > 0) {
            speed_right = (speed_right > 0) ? m_min_speed : -m_min_speed;
        }
        
        // Nastavení výkonu motorů
        man.motor(m_id_left).speed(pctToSpeed(speed_left));
        man.motor(m_id_right).speed(pctToSpeed(speed_right));
        
        // std::cout << "Speed left: " << speed_left << ", Speed right: " << speed_right << std::endl;
        // std::cout << "Progress: " << (avg_progress * 100.0f) << "%" << std::endl;
        // std::cout << "----------------------------------------" << std::endl;
    }
    
    // Zastavení motorů
    man.motor(m_id_left).speed(0);
    man.motor(m_id_right).speed(0);
    man.motor(m_id_left).power(0);
    man.motor(m_id_right).power(0);
    
    // std::cout << "forward_acc UKONČENO" << std::endl;
}


void Motors::back_buttons(float speed, std::function<bool()> first_button, std::function<bool()> second_button) {
    auto& man = rb::Manager::get();
    
    float m_kp = 0.23f; // Proporcionální konstanta
    float m_min_speed = 15.0f; // Minimální rychlost motorů
    float m_max_correction = 5.0f; // Maximální korekce rychlosti

    bool left_done = false;
    bool right_done = false;
    
    // Reset pozic
    man.motor(m_id_left).setCurrentPosition(0);
    man.motor(m_id_right).setCurrentPosition(0);
    
    int left_pos = 0;
    int right_pos = 0;

    byte step = 3;
    // Základní rychlosti s přihlédnutím k polaritě
    float base_speed_left = m_polarity_switch_left ? speed : -speed;
    float base_speed_right = m_polarity_switch_right ? speed : -speed;

    float step_left = (base_speed_left > 0) ? step : -step;
    float step_right = (base_speed_right > 0) ? step : -step;

    float current_speed_left = (base_speed_left > 0) ? m_min_speed : - m_min_speed;
    float current_speed_right = (base_speed_right > 0) ? m_min_speed : - m_min_speed;

    byte a = 0;
    
    unsigned long start_time = millis();
    int timeoutMs = 10000;
    
    while((millis() - start_time) < timeoutMs) {
        
        // Čtení pozic
        man.motor(m_id_left).requestInfo([&](rb::Motor& info) {
             left_pos = info.position();
          });
        man.motor(m_id_right).requestInfo([&](rb::Motor& info) {
             right_pos = info.position();
          });

        delay(10);

        // std::cout << "Left pos: " << left_pos << ", Right pos: " << right_pos << std::endl;
        
        //Zrychlování
        if((abs(current_speed_left) < abs(speed)) && (abs(current_speed_right) < abs(speed)) && (a % 9 ==0)){
            current_speed_left += step_left;
            current_speed_right += step_right;
            a=0;
        }
        a++;
        // std::cout<<"current_speed_left : "<< current_speed_left<<std::endl;
        // P regulátor - pracuje s absolutními hodnotami pozic
        int error = abs(left_pos)  - abs(right_pos);

        float correction = (error) * m_kp;
        correction = std::max(-m_max_correction, std::min(correction, m_max_correction));
        
        // Výpočet korigovaných rychlostí
        float speed_left = current_speed_left;
        float speed_right = current_speed_right;
        
        // Aplikace korekce podle polarity
        if (error > 0) {
            // Levý je napřed - zpomalit levý
            if (m_polarity_switch_left) {
                speed_left -= correction;  // Přidat k záporné rychlosti = zpomalit
            } else {
                speed_left += correction;  // Odečíst od kladné rychlosti = zpomalit
            }
        } else if (error < 0) {
            // Pravý je napřed - zpomalit pravý
            if (m_polarity_switch_right) {
                speed_right += correction;  // Odečíst od záporné rychlosti = zpomalit
            } else {
                speed_right -= correction;  // Přidat ke kladné rychlosti = zpomalit
            }
        }
        
        // Zajištění minimální rychlosti
        if (abs(speed_left) < m_min_speed) {
            speed_left = (speed_left > 0) ? m_min_speed : -m_min_speed;
        }
        if (abs(speed_right) < m_min_speed) {
            speed_right = (speed_right > 0) ? m_min_speed : -m_min_speed;
        }
        
        // Nastavení výkonu motorů
        man.motor(m_id_left).speed(pctToSpeed(speed_left));
        man.motor(m_id_right).speed(pctToSpeed(speed_right));
        // std::cout << "Speed left: " << speed_left << ", Speed right: " << speed_right << std::endl;

        if(first_button() && !left_done) {
            //std::cout << "TLACITKO 1 STISKNUTO" << std::endl;
            start_time = millis();
            timeoutMs = 3000;
            left_done = true;
        }
        if(second_button() && !right_done) {
            //std::cout << "TLACITKO 2 STISKNUTO" << std::endl;
            start_time = millis();
            timeoutMs = 3000;
            right_done = true;
        }
        if(left_done && right_done ) {
            //std::cout << "OBE TLACITKA Stisknuta" << std::endl;
            delay(50);
            break;
        }
    }
    
    // Zastavení motorů
    man.motor(m_id_left).speed(0);
    man.motor(m_id_left).speed(0);
    man.motor(m_id_left).power(0);
    man.motor(m_id_right).power(0);
}

void Motors::front_buttons(float speed, std::function<bool()> first_button, std::function<bool()> second_button) {
    auto& man = rb::Manager::get();
    
    float m_kp = 0.23f; // Proporcionální konstanta
    float m_min_speed = 15.0f; // Minimální rychlost motorů
    float m_max_correction = 5.0f; // Maximální korekce rychlosti

    bool left_done = false;
    bool right_done = false;
    
    // Reset pozic
    man.motor(m_id_left).setCurrentPosition(0);
    man.motor(m_id_right).setCurrentPosition(0);
    
    int left_pos = 0;
    int right_pos = 0;

    byte step = 3;
    // Základní rychlosti s přihlédnutím k polaritě (jízda dopředu)
    float base_speed_left = m_polarity_switch_left ? -speed : speed;
    float base_speed_right = m_polarity_switch_right ? -speed : speed;

    float step_left = (base_speed_left > 0) ? step : -step;
    float step_right = (base_speed_right > 0) ? step : -step;

    float current_speed_left = (base_speed_left > 0) ? m_min_speed : - m_min_speed;
    float current_speed_right = (base_speed_right > 0) ? m_min_speed : - m_min_speed;

    byte a = 0;
    
    unsigned long start_time = millis();
    int timeoutMs = 10000;
    
    while((millis() - start_time) < timeoutMs) {
        
        // Čtení pozic
        man.motor(m_id_left).requestInfo([&](rb::Motor& info) {
             left_pos = info.position();
          });
        man.motor(m_id_right).requestInfo([&](rb::Motor& info) {
             right_pos = info.position();
          });

        delay(10);
        
        //Zrychlování
        if((abs(current_speed_left) < abs(speed)) && (abs(current_speed_right) < abs(speed)) && (a % 9 ==0)){
            current_speed_left += step_left;
            current_speed_right += step_right;
            a=0;
        }
        a++;
        
        // P regulátor - pracuje s absolutními hodnotami pozic
        int error = abs(left_pos)  - abs(right_pos);

        float correction = (error) * m_kp;
        correction = std::max(-m_max_correction, std::min(correction, m_max_correction));
        
        // Výpočet korigovaných rychlostí
        float speed_left = current_speed_left;
        float speed_right = current_speed_right;
        
        // Aplikace korekce podle polarity pro jízdu vpred
        if (error > 0) {
            // Levý je napřed - zpomalit levý
            if (m_polarity_switch_left) {
                speed_left += correction; 
            } else {
                speed_left -= correction;  
            }
        } else if (error < 0) {
            // Pravý je napřed - zpomalit pravý
            if (m_polarity_switch_right) {
                speed_right -= correction;  
            } else {
                speed_right += correction;  
            }
        }
        
        // Zajištění minimální rychlosti
        if (abs(speed_left) < m_min_speed) {
            speed_left = (speed_left > 0) ? m_min_speed : -m_min_speed;
        }
        if (abs(speed_right) < m_min_speed) {
            speed_right = (speed_right > 0) ? m_min_speed : -m_min_speed;
        }
        
        // Nastavení výkonu motorů
        man.motor(m_id_left).speed(pctToSpeed(speed_left));
        man.motor(m_id_right).speed(pctToSpeed(speed_right));

        if(first_button() && !left_done) {
            start_time = millis();
            timeoutMs = 3000;
            left_done = true;
        }
        if(second_button() && !right_done) {
            start_time = millis();
            timeoutMs = 3000;
            right_done = true;
        }
        if(left_done && right_done ) {
            delay(50);
            break;
        }
    }
    
    // Zastavení motorů
    man.motor(m_id_left).speed(0);
    man.motor(m_id_right).speed(0);
    man.motor(m_id_left).power(0);
    man.motor(m_id_right).power(0);
}

void Motors::wall_following(float distance_to_drive, float speed, bool automatic_distance_of_wall, float distance_of_wall, bool is_wall_on_right,
                   std::function<int()> first_sensor, 
                   std::function<int()> second_sensor, int o_kolik_je_zadni_dal) {

    auto& man = rb::Manager::get();
    
    float m_kp = 0.06f; // Proporcionální konstanta
    float m_min_speed = 20.0f; // Minimální rychlost motorů
    float m_max_correction = 1.2f; // Maximální korekce rychlosti
    
    // Reset pozic
    man.motor(m_id_left).setCurrentPosition(0);
    man.motor(m_id_right).setCurrentPosition(0);
    
    int left_pos = 0;
    int right_pos = 0;

    float speed_left = 0;
    float speed_right = 0;
    // Základní rychlosti s přihlédnutím k polaritě
    float base_speed_left = m_polarity_switch_left ? -speed : speed;
    float base_speed_right = m_polarity_switch_right ? -speed : speed;
    
    unsigned long start_time = millis();
    int timeoutMs = timeout_ms(distance_to_drive, speed);
    int target_ticks = mmToTicks(distance_to_drive);
    float celkovy_error = 0;

    if(automatic_distance_of_wall){
        distance_of_wall = first_sensor();
    }


    while((abs(target_ticks) > abs(left_pos) || abs(target_ticks) > abs(right_pos)) && 
          (millis() - start_time < timeoutMs)) {
        
        man.motor(m_id_left).requestInfo([&](rb::Motor& info) {
             left_pos = info.position();
          });
        man.motor(m_id_right).requestInfo([&](rb::Motor& info) {
             right_pos = info.position();
          });

        delay(50);

        int front_distance_senzor = first_sensor();
        
        int back_distance_senzor = second_sensor() + o_kolik_je_zadni_dal;

        if(front_distance_senzor <= 0 || front_distance_senzor > 300 || back_distance_senzor <= 0 || back_distance_senzor > 300){
            // std::cout << "Chyba senzoru vzdálenosti!" << std::endl;
            front_distance_senzor = distance_of_wall;
            back_distance_senzor = distance_of_wall;
        }

        celkovy_error = (front_distance_senzor + back_distance_senzor)/2 - distance_of_wall;
        float error = 0;
        if(abs(celkovy_error) > 40){
            error = celkovy_error * 0.8f;
            // std::cout << "Velká chyba: " << error << std::endl;
        }
        else{
            error =  (front_distance_senzor - back_distance_senzor) * 1.8 + celkovy_error * 0.5f;
            // std::cout << "Malá chyba: " << error << std::endl;
        }

        float correction = error * m_kp;
        correction = abs(std::max(-m_max_correction, std::min(correction, m_max_correction)));
        speed_left = base_speed_left;
        speed_right = base_speed_right;

        // Aplikace korekce podle polarity
        if(is_wall_on_right){
            if(speed > 0){
                if (error > 0) {
                    if (m_polarity_switch_left) {
                        speed_left -= correction;
                        speed_right -= correction;
                    } else {
                        speed_left += correction;
                        speed_right += correction;
                    }
                } else if (error < 0) {
                    if (m_polarity_switch_right) {
                        speed_right -= correction;
                        speed_left -= correction;
                    } else {
                        speed_right += correction;
                        speed_left += correction;
                    }
                }
            }
            else{
                if (error > 0) {
                    if (m_polarity_switch_left) {
                        speed_left += correction;
                        speed_right += correction;
                    } else {
                        speed_left -= correction;
                        speed_right -= correction;
                    }
                } else if (error < 0) {
                    if (m_polarity_switch_right) {
                        speed_right += correction;
                        speed_left += correction;
                    } else {
                        speed_right -= correction;
                        speed_left -= correction;
                    }
                }
            }
        }
        else{
            if(speed > 0){
                if (error > 0) {
                    if (m_polarity_switch_left) {
                        speed_left += correction;
                        speed_right += correction;//ok
                    } else {
                        speed_left -= correction;
                        speed_right -= correction;//ok
                    }
                } else if (error < 0) {
                    if (m_polarity_switch_right) {
                        speed_right += correction;
                        speed_left += correction;//ok
                    } else {
                        speed_right -= correction;
                        speed_left -= correction;//ok
                    }
                }
            }
            else{
                if (error > 0) {
                    if (m_polarity_switch_left) {
                        speed_left -= correction;
                        speed_right -= correction;//ok
                    } else {
                        speed_left += correction;
                        speed_right += correction;//ok
                    }
                } else if (error < 0) {
                    if (m_polarity_switch_right) {
                        speed_right -= correction;
                        speed_left -= correction;//ok
                    } else {
                        speed_right += correction;
                        speed_left += correction;//ok
                    }
                }
            }
        }
        // Zajištění minimální rychlosti
        if (abs(speed_left) < m_min_speed) {
            speed_left = (speed_left > 0) ? m_min_speed : -m_min_speed;
        }
        if (abs(speed_right) < m_min_speed) {
            speed_right = (speed_right > 0) ? m_min_speed : -m_min_speed;
        }

        // Nastavení výkonu motorů
        man.motor(m_id_left).speed(pctToSpeed(speed_left));
        man.motor(m_id_right).speed(pctToSpeed(speed_right));
        
        // Výpis informací pro ladění (volitelné)
        // std::cout << "----------------------------------------" << std::endl;
        // std::cout << "celkovy_error: " << celkovy_error << std::endl;
        // std::cout << "Error: " << error << std::endl;
        // std::cout << "Correction: " << correction << std::endl;
        // std::cout << "Speed left: " << speed_left << ", Speed right: " << speed_right << std::endl;
        // std::cout << "Front sensor: " << front_distance_senzor << ", Back sensor: " << back_distance_senzor << std::endl;
        // std::cout << "----------------------------------------" << std::endl;
        // // printf_wifi("celkovy error:  %.1f", celkovy_error);
        // // printf_wifi("Error:  %.1f", error);
        // // printf_wifi("Correction:  %.1f", correction);
        // // printf_wifi("Speed left:  %.1f", speed_left);
        // // printf_wifi("Speed right:  %.1f", speed_right);
        // // printf_wifi("Front sensor:  %d", front_distance_senzor);
        // // printf_wifi("Back sensor:  %d", back_distance_senzor);
        // // printf_wifi("-----------------------------");
        // // handleWebClients();
        // Krátké zpoždění pro stabilizaci
        delay(50);
        }
    
    // Zastavení motorů po ukončení
    man.motor(m_id_left).speed(0);
    man.motor(m_id_right).speed(0);
    man.motor(m_id_left).power(0);
    man.motor(m_id_right).power(0);
    
}

void Motors::orient_to_wall(bool button_or_right, std::function<int()> first_sensor, //first senzor se musi vzdy davat do prava, nebo do predu!!!
                                   std::function<int()> second_sensor, int o_kolik_je_dal_zadni ,float speed) { 
    int distance_first = 0;
    int distance_second = 0;

    auto& man = rb::Manager::get();

    float speed_left = speed;
    float speed_right = speed;


    for(byte i = 0; i < 3; i++){
        distance_first = first_sensor();
        distance_second = second_sensor();
        delay(60);
    }

    distance_first = first_sensor();
    delay(80);
    distance_second = second_sensor();
    // std::cout<< "Distance first: " << distance_first << " , Distance second: " << distance_second <<std::endl;
    if(distance_first > 800 && distance_second > 800 + o_kolik_je_dal_zadni){
        // std::cout<< "Jsme moc daleko, nebo nevim jak se mam srovnat"<<std::endl;
        return;
    }

    int vzdalenost_first = distance_first;
    int vzdalenost_second = distance_second - o_kolik_je_dal_zadni;

    int start_error = vzdalenost_first - vzdalenost_second;
    
    if(button_or_right){
        if(vzdalenost_first > vzdalenost_second){ //pray kolo couve a levy jede dopredu
            if(m_polarity_switch_right){
                speed_right = speed;
                speed_left = speed;
            }
            else{
                speed_right = -speed;
                speed_left = -speed;
            }
        }
        else{ //levy kolo couve a pravy jede dopredu
            if(m_polarity_switch_right){
                speed_left = -speed;
                speed_right = -speed;
            }
            else{
                speed_left = speed;
                speed_right = speed;
            }
        }
    }
    else{
        if(vzdalenost_first > vzdalenost_second
        ){ //pray kolo jede dopredu a levy couve
            if(m_polarity_switch_right){
                speed_right = -speed;
                speed_left = -speed;
            }
            else{
                speed_right = speed;
                speed_left = speed;
            }
        }
        else{ //levy kolo jede dopredu a pravy couve
            if(m_polarity_switch_right){
                speed_left = speed;
                speed_right = speed;
            }
            else{
                speed_left = -speed;
                speed_right = -speed;
            }
        }
    }
    unsigned long start_time = millis();
    int timeut_ms = 5000;

    while(timeut_ms > millis() - start_time){
        distance_first = first_sensor();
        distance_second = second_sensor();

        int vzdalenost_first = distance_first;  
        int vzdalenost_second = distance_second - o_kolik_je_dal_zadni;

        int error = vzdalenost_first - vzdalenost_second;

        if(abs(error) <= 5){ //nastavit
            break;
        }

        if((start_error > 0) && (error > 0)){ //porad stejne
            man.motor(m_id_left).speed(pctToSpeed(speed_left));
            man.motor(m_id_right).speed(pctToSpeed(speed_right));
        }
        else if((start_error < 0) && (error < 0)){ //porad stejne
            man.motor(m_id_left).speed(pctToSpeed(speed_left));
            man.motor(m_id_right).speed(pctToSpeed(speed_right));
        }
        else{
            man.motor(m_id_left).speed(pctToSpeed(-speed_left)); // prejel to a musi jet zpet
            man.motor(m_id_right).speed(pctToSpeed(-speed_right));
        }

        delay(30);
    }
    man.motor(m_id_left).speed(0);
    man.motor(m_id_right).speed(0);
    man.motor(m_id_left).power(0);
    man.motor(m_id_right).power(0);

}

void Motors::orient_to_wall_any_price(bool button_or_right, std::function<uint32_t()> first_sensor, 
                   std::function<uint32_t()> second_sensor, int o_kolik_je_dal_zadni, float speed){

    int distance_first = 0;
    int distance_second = 0;

    int target_ticks = 1.1 * mmToTicks((M_PI * roztec_kol));

    auto& man = rb::Manager::get();


    // Reset pozic
    man.motor(m_id_left).setCurrentPosition(0);
    man.motor(m_id_right).setCurrentPosition(0);

    for(byte i = 0; i < 3; i++){
        distance_first = first_sensor();
        distance_second = second_sensor();
        delay(60);
    }

    distance_first = first_sensor();
    delay(80);
    distance_second = second_sensor();
    // std::cout<< "Distance first: " << distance_first << " , Distance second: " << distance_second <<std::endl;

    if(distance_first > 800 && distance_second > 800 + o_kolik_je_dal_zadni){
        // std::cout<< "Jsme moc daleko, nebo nevim jak se mam srovnat"<<std::endl;
    }
    else if((distance_first +5 > distance_second - o_kolik_je_dal_zadni) && (distance_first -5 < distance_second - o_kolik_je_dal_zadni) && (distance_first < 1000) && (distance_second < 1000) && (distance_first > 0 && (distance_second > 0))){
        // std::cout<<"UZ jsem vyrovnanej, kaslu an to"<<std::endl;
        return;
    }

    man.motor(m_id_left).speed(pctToSpeed(speed)); // jeden je prehozene tak to vzdy nekam se otoci
    man.motor(m_id_right).speed(pctToSpeed(speed));

    int dobra_pos_left = 0;

    int left_pos = 0;
    int right_pos = 0;
    int vzdalenost_left;
    int vzdalenost_right;

    float avg_vzdalenost = 1000.0f;
    float nejmensi_avg_vzdalenost = 10000.0f;

    unsigned long start_time = millis();
    int timeut_ms = 8000;

    while(timeut_ms > millis() - start_time){

        man.motor(m_id_left).requestInfo([&](rb::Motor& info) {
             left_pos = info.position();
          });
        man.motor(m_id_right).requestInfo([&](rb::Motor& info) {
             right_pos = info.position();
          });

        delay(50);

        vzdalenost_left = first_sensor();
        vzdalenost_right = second_sensor() - o_kolik_je_dal_zadni;

        if(vzdalenost_left == 0 || vzdalenost_right == 0){
            avg_vzdalenost = 1000.0f;
        }
        else{
            avg_vzdalenost = float(vzdalenost_left + vzdalenost_right) / 2;
        }

        if(avg_vzdalenost < nejmensi_avg_vzdalenost){
            nejmensi_avg_vzdalenost = avg_vzdalenost;
            dobra_pos_left = left_pos;
        }

        if(abs(left_pos) > target_ticks && abs(right_pos) > target_ticks){
            break;
        }
    }
    man.motor(m_id_left).speed(0);
    man.motor(m_id_right).speed(0);
    man.motor(m_id_left).power(0);
    man.motor(m_id_right).power(0);

    // std::cout<<"Uz to mam objety"<<std::endl;

    // std::cout<<"nejmensi_avg_vzdalenost"<< nejmensi_avg_vzdalenost << std::endl;

    // std::cout<<"dobra pozice eft"<< dobra_pos_left << std::endl;

    // std::cout<<"aktualnipozice left"<< left_pos << std::endl;

    delay(300);

    man.motor(m_id_left).speed(pctToSpeed(-speed));
    man.motor(m_id_right).speed(pctToSpeed(-speed));

    start_time = millis();

    while(timeut_ms > millis() - start_time){
        man.motor(m_id_left).requestInfo([&](rb::Motor& info) {
             left_pos = info.position();
          });
        man.motor(m_id_right).requestInfo([&](rb::Motor& info) {
             right_pos = info.position();
          });

        delay(10);

        if(left_pos > 0){
            if(left_pos < dobra_pos_left- 50){
                break;
            }
        }
        else{
            if(left_pos > dobra_pos_left + 50){
                break;
            }
        }
    }

    // std::cout<<"Uz jsem tam"<<std::endl;

    man.motor(m_id_left).speed(0);
    man.motor(m_id_right).speed(0);
    man.motor(m_id_left).power(0);
    man.motor(m_id_right).power(0);

    delay(300);

    orient_to_wall(button_or_right, first_sensor, second_sensor, o_kolik_je_dal_zadni, speed);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

int32_t Motors::scale(int32_t val) {
    return val * 100 / RBPROTOCOL_AXIS_MAX;
}

int16_t Motors::pctToPower(int8_t pct) {
    return rb::clamp(pct * -INT16_MIN / 100, INT16_MIN, INT16_MAX);
}

int16_t Motors::pctToSpeed(float pct) {
    // Omezení vstupu
    pct = rb::clamp(pct, -100.0f, 100.0f);
    
    // Přímý výpočet s přetypováním až na konci
    int32_t speed = static_cast<int32_t>((pct * m_max_speed) / 100.0f);
    
    // Omezení rozsahu
    return rb::clamp(speed, -INT16_MAX, INT16_MAX);
}

int32_t Motors::mmToTicks(float mm) const {
    return (mm / m_wheel_circumference) * prevod_motoru;
}

float Motors::ticksToMm(int32_t ticks) const {
    return float(ticks) / prevod_motoru * m_wheel_circumference;
}
int32_t Motors::mmToTicks_left(float mm) const {
    return (mm / m_wheel_circumference_left) * prevod_motoru;
}
int32_t Motors::mmToTicks_right(float mm) const {
    return (mm / m_wheel_circumference_right) * prevod_motoru;
}
float Motors::ticksToMm_left(int32_t ticks) const {
    return float(ticks) / prevod_motoru * m_wheel_circumference_left;
}
float Motors::ticksToMm_right(int32_t ticks) const {
    return float(ticks) / prevod_motoru * m_wheel_circumference_right;
}


// WiFi inicializace
void Motors::initWifi(const char* ssid, const char* password) {
    std::cout << "Inicializace WiFi..." << std::endl;
    
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(1000);
        attempts++;
        std::cout << ".";
    }
    std::cout << std::endl;
    
    if (WiFi.status() == WL_CONNECTED) {
        m_wifi_initialized = true;
        m_server = new WebServer(8080);
        
        std::cout << "WiFi pripojeno!" << std::endl;
        std::cout << "IP: " << WiFi.localIP().toString().c_str() << std::endl;
        std::cout << "Web server spusten na: http://" << WiFi.localIP().toString().c_str() << ":8080" << std::endl;
        
        // Nastavení webových stránek - OPRAVENÁ VERZE
        m_server->on("/", [this]() {
            std::cout << "HTTP GET / received" << std::endl;
            String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Robot Log</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 20px;
            background: white;
            color: black;
        }
        #log {
            background: white;
            border: 1px solid #ccc;
            padding: 10px;
            height: 80vh;
            overflow-y: auto;
            font-family: monospace;
            white-space: pre-wrap;
            line-height: 1.3;
        }
        .controls {
            margin: 10px 0;
        }
        button {
            padding: 8px 15px;
            margin-right: 10px;
            cursor: pointer;
            background: #f0f0f0;
            border: 1px solid #ccc;
            border-radius: 3px;
        }
        button:hover {
            background: #e0e0e0;
        }
    </style>
</head>
<body>
    <h1>Robot Log</h1>
    <div><strong>Adresa:</strong> http://)rawliteral" + WiFi.localIP().toString() + R"rawliteral(:8080</div>
    
    <div class="controls">
        <button onclick="clearLog()">Smazat log</button>
        <button onclick="toggleAutoRefresh()" id="refreshBtn">Vypnout auto-obnovování</button>
    </div>
    
    <div id="log">Načítám...</div>

    <script>
        let autoRefresh = true;
        let refreshInterval = setInterval(updateLog, 500);
        let shouldScroll = true;
        
        // Sleduj uživatelský scroll
        const logElement = document.getElementById('log');
        logElement.addEventListener('scroll', function() {
            // Pokud uživatel scrolluje nahoru, zastav auto-scroll
            shouldScroll = (logElement.scrollTop + logElement.clientHeight >= logElement.scrollHeight - 10);
        });
        
        function updateLog() {
            if (!autoRefresh) return;
            
            fetch('/log')
                .then(response => response.text())
                .then(data => {
                    const wasScrolledToBottom = shouldScroll;
                    logElement.textContent = data;
                    
                    // Scrolluj dolů pouze pokud byl uživatel na konci
                    if (wasScrolledToBottom) {
                        logElement.scrollTop = logElement.scrollHeight;
                    }
                })
                .catch(err => console.error('Chyba:', err));
        }
        
        function clearLog() {
            fetch('/clear')
                .then(() => updateLog())
                .catch(err => console.error('Chyba:', err));
        }
        
        function toggleAutoRefresh() {
            autoRefresh = !autoRefresh;
            const button = document.getElementById('refreshBtn');
            
            if (autoRefresh) {
                button.textContent = 'Vypnout auto-obnovování';
                refreshInterval = setInterval(updateLog, 500);
            } else {
                button.textContent = 'Zapnout auto-obnovování';
                clearInterval(refreshInterval);
            }
        }
        
        // První načtení
        updateLog();
    </script>
</body>
</html>
)rawliteral";
            m_server->send(200, "text/html", html);
        });
        
        m_server->on("/log", [this]() {
            m_server->send(200, "text/plain", m_wifi_log_buffer);
        });
        
        m_server->on("/clear", [this]() {
            m_wifi_log_buffer = "";
            m_server->send(200, "text/plain", "Log smazán");
        });

        m_server->on("/test", [this]() {
            m_server->send(200, "text/plain", "Robot WiFi test OK!");
        });

        m_server->onNotFound([this]() {
            m_server->send(404, "text/plain", "Cesta nenalezena");
        });
        
        m_server->begin();
        // printf_wifi("Web server started on http://%s:8080", WiFi.localIP().toString().c_str());
    } else {
        std::cout << "WiFi se nepodařilo připojit!" << std::endl;
    }
}

// Základní print funkce
void Motors::print_wifi(const String& message) {
    if (!m_wifi_initialized) return;
    
    // Pokud zpráva obsahuje \n, rozděl ji na více řádků
    int startIndex = 0;
    int newLineIndex;
    
    do {
        newLineIndex = message.indexOf('\n', startIndex);
        String line;
        
        if (newLineIndex == -1) {
            line = message.substring(startIndex);
        } else {
            line = message.substring(startIndex, newLineIndex);
        }
        
        // Přidat časové razítko pouze pokud řádek není prázdný
        if (line.length() > 0) {
            String timestamp = "[" + String(millis() / 1000.0, 1) + "s] ";
            m_wifi_log_buffer += timestamp + line + "\n";
        }
        
        startIndex = newLineIndex + 1;
    } while (newLineIndex != -1);
    
    // ZVĚTŠENÝ buffer - 20 000 znaků
    if (m_wifi_log_buffer.length() > 40000) {
        int newStart = m_wifi_log_buffer.indexOf('\n', m_wifi_log_buffer.length() - 15000);
        if (newStart != -1) {
            m_wifi_log_buffer = m_wifi_log_buffer.substring(newStart + 1);
        }
    }
}


// Přetížení pro const char*
void Motors::print_wifi(const char* message) {
    print_wifi(String(message));
}

// Formátovaný výpis (printf style)
void Motors::printf_wifi(const char* format, ...) {
    if (!m_wifi_initialized) return;
    
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    print_wifi(String(buffer));
}

// Zpracování webových požadavků
void Motors::handleWebClient() {
    if (m_wifi_initialized && m_server) {
        m_server->handleClient();
    }
}

bool Motors::rkButton1(bool waitForRelease) {
    bool isPressed = (digitalRead(Button1) == LOW);
    if (isPressed && waitForRelease) {
        while(digitalRead(Button1) == LOW) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
    return isPressed;
}

bool Motors::rkButton2(bool waitForRelease) {
    bool isPressed = (digitalRead(Button2) == LOW);
    if (isPressed && waitForRelease) {
        while(digitalRead(Button2) == LOW) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
    return isPressed;
}

}; // namespacer rk
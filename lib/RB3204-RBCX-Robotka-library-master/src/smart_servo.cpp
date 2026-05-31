#include "smart_servo.h"

namespace rk {
    // Inicializace statických proměnných
    lx16a::SmartServoBus* smart_servo::bus = nullptr;

    void smart_servo::init(const rkConfig& cfg){
        bus = &rkSmartServoBus(cfg.pocet_chytrych_serv);
    }
    
    void smart_servo::rkSmartServoInit(int id, int low, int high, int16_t max_diff_centideg, uint8_t  max_diff_readings) {
        if (bus == nullptr) {
            printf("Chyba: Smart servo bus není inicializován! Voláte smart_servo::init()?\n");
            return;
        }
        bus->setAutoStop(id, false);
        bus->limit(id, lx16a::Angle::deg(low), lx16a::Angle::deg(high));
        bus->setAutoStopParams(
            lx16a::SmartServoBus::AutoStopParams{
                .max_diff_centideg = max_diff_centideg,
                .max_diff_readings = max_diff_readings,
            });
        printf("Smart Servo %d inicializováno (limity: %d°-%d°)\n", id, low, high);
    }

    void smart_servo::rkSmartServoMove(int id, int angle, int speed) {
        if (bus == nullptr) {
            printf("Chyba: Smart servo bus není inicializován!\n");
            return;
        }
        if (angle < 0 || angle > 240) {
            printf("Chyba: Úhel musí být v rozsahu 0–240 stupňů.\n");
            return;
        }
        bus->setAutoStop(id, false);
        bus->set(id, lx16a::Angle::deg(angle), speed);
        printf("Smart Servo %d move na %d stupňů rychlostí %d\n", id, angle, speed);
    }

    void smart_servo::rkSmartServoSoftMove(int id, int angle, int speed) {
        if (bus == nullptr) {
            printf("Chyba: Smart servo bus není inicializován!\n");
            return;
        }
        if (angle < 0 || angle > 240) {
            Serial.println("Chyba: Úhel musí být v rozsahu 0–240 stupňů.");
            return;
        }
        bus->setAutoStop(id, true);
        bus->set(id, lx16a::Angle::deg(angle), speed);
        printf("Smart Servo %d soft_move na %d stupňů rychlostí %d\n", id, angle, speed);
    }

    byte smart_servo::rkSmartServosPosicion(int id) {
        if (bus == nullptr) {
            printf("Chyba: Smart servo bus není inicializován!\n");
            return 0;
        }
        float angle = bus->pos(id).deg();
        if (angle < 0) return 0;
        if (angle > 240) return 240;
        printf("Pozice serva na id: %d je: %d\n", id, (byte)angle);
        return (byte)angle;
    }
}
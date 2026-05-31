#pragma once

#include "robotka.h"

namespace rk {
    class smart_servo {
    public:
        static void init(const rkConfig& cfg);
        static void rkSmartServoInit(int id, int low, int high, int16_t max_diff_centideg, uint8_t  max_diff_readings);
        static void rkSmartServoMove(int id, int angle, int speed = 200);
        static void rkSmartServoSoftMove(int id, int angle, int speed = 200);
        static byte rkSmartServosPosicion(int id);

    private:
        static lx16a::SmartServoBus* bus;
    };
};
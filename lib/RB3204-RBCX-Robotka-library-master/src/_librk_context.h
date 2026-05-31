#pragma once

#include <atomic>

#include "_librk_motors.h"
#include "wifi_control.h"
#include "smart_servo.h"

namespace rk {

class Context {
public:
    Context();
    ~Context();

    void setup(const rkConfig& cfg);

    rb::Protocol* prot() const { return m_prot; }
    Motors& motors() { return m_motors; }

    Wifi& wifi() { return m_wifi; }

    smart_servo& smart_s() {return m_smart_s;}

    adc1_channel_t irChanLeft() const { return m_ir_left; }
    adc1_channel_t irChanRight() const { return m_ir_right; }

    uint16_t irRead(adc1_channel_t chan, uint16_t samples = 32);

    void stupidServoSet(uint8_t id, float positionDegrees);
    float stupidServoGet(uint8_t id);

private:

    void initIrSensors();

    Motors m_motors;
    Wifi m_wifi;
    smart_servo m_smart_s;
    rb::Protocol* m_prot;

    std::atomic<bool> m_initialized;

    std::atomic<bool> m_ir_installed;


    adc1_channel_t m_ir_left;
    adc1_channel_t m_ir_right;

    float m_stupid_servo_min;
    float m_stupid_servo_max;
};

extern Context gCtx;

};

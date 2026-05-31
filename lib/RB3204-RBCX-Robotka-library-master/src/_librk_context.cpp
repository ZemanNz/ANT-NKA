#include "nvs_flash.h"

#include "robotka.h"

#include "RBCX.h"

#include "rbprotocol.h"
#include "rbdns.h"
#include <stdio.h>

#include "_librk_context.h"

using namespace rb;

#define TAG "robotka"

namespace rk {

Context gCtx;

Context::Context() {
    m_prot = nullptr;
}

Context::~Context() {
}

void Context::setup(const rkConfig& cfg) {
    bool expected = false;
    if (!m_initialized.compare_exchange_strong(expected, true)) {
        ESP_LOGE(TAG, "rkSetup was called more than once, this is WRONG!");
        return;
    }

    rb::Timers::deleteFreeRtOsTimerTask();

    // Initialize the robot manager
    auto& man = Manager::get();

    auto man_flags = MAN_NONE;
    

    if (!cfg.motor_enable_failsafe) {
        man_flags = ManagerInstallFlags(man_flags | MAN_DISABLE_MOTOR_FAILSAFE);
    }

    man.install(man_flags, 6 * 1024);

    m_ir_left = cfg.pins.ir_adc_chan_left;
    m_ir_right = cfg.pins.ir_adc_chan_right;

    pinMode(cfg.Button1,INPUT_PULLUP);
    pinMode(cfg.Button2,INPUT_PULLUP);

    m_stupid_servo_min = cfg.stupid_servo_min;
    m_stupid_servo_max = cfg.stupid_servo_max;
    if(cfg.pocet_chytrych_serv > 0){
        m_smart_s.init(cfg);
    }

    m_motors.init(cfg);

    if(cfg.enable_wifi_control_wasd && cfg.wifi_ssid && cfg.wifi_password){
        m_wifi.setupWiFiControl(cfg);
    }

    if(cfg.enable_wifi_terminal && cfg.wifi_ssid && cfg.wifi_password){
        m_wifi.setupWiFiControl(cfg);
    }
    const auto& v = man.coprocFwVersion();
    printf("STM32 FW version: %06x %.8s%s\n", v.number, v.revision,
        v.dirty ? "-dirty" : "");
}

void Context::initIrSensors() {
    adc1_config_width(ADC_WIDTH_12Bit);
    adc1_config_channel_atten(m_ir_left, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(m_ir_right, ADC_ATTEN_DB_11);
}

uint16_t Context::irRead(adc1_channel_t chan, uint16_t samples) {
    bool ex = false;
    if (m_ir_installed.compare_exchange_strong(ex, true)) {
        initIrSensors();
    }

    uint32_t reading = 0;
    for (uint16_t i = 0; i < samples; ++i) {
        reading += adc1_get_raw(chan);
    }
    return reading / samples;
}

void Context::stupidServoSet(uint8_t id, float positionDegrees) {
    const auto coef = rb::clamp(positionDegrees, -90.f, 90.f) / 90.f;
    const auto val = coef >= 0.f ? coef * m_stupid_servo_max : coef * m_stupid_servo_min * -1;
    Manager::get().stupidServo(id).setPosition(val);
}

float Context::stupidServoGet(uint8_t id) {
    const auto val = Manager::get().stupidServo(id).position();
    if (std::isnan(val))
        return val;

    if (val >= 0.f) {
        return val / m_stupid_servo_max * 90.f;
    } else {
        return val / m_stupid_servo_min * -90.f;
    }
}

}; // namespace rk

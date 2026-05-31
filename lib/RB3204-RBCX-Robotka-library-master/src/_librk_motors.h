#pragma once

#include <functional>
#include <list>
#include <mutex>
#include <stdint.h>

#include <WiFi.h>
#include <WebServer.h>

#include "RBCXPinout.h"
#include "robotka.h"

namespace rk {

class Motors {
public:
    Motors();
    ~Motors();

    typedef std::function<void()> dual_callback_t;

    void init(const rkConfig& cfg);

    void setPower(int8_t left, int8_t right);
    void setPower(int8_t left, int8_t right, uint8_t pwm_pct_left, uint8_t pwm_pct_right);
    void setPowerById(rb::MotorId id, int8_t power);

    void setSpeed(int8_t left, int8_t right);
    void setSpeed(int8_t left, int8_t right, uint8_t pwm_pct_left, uint8_t pwm_pct_right);
    void setSpeedById(rb::MotorId id, int8_t speed);
    void drive(float left, float right, float speed_left, float speed_right,  dual_callback_t callback = nullptr);
    void driveById(rb::MotorId id, float mm, uint8_t speed, std::function<void()> callback = nullptr);

    float position(rb::MotorId id);
    void setPosition(rb::MotorId id, float positionMm);

    void joystick(int32_t x, int32_t y);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    int timeout_ms(float mm, float speed);
    int16_t max_rychlost();
    void forward(float mm, float speed);
    void backward(float mm, float speed);
    void turn_on_spot_left(float angle, float speed);
    void turn_on_spot_right(float angle, float speed);
    void radius_right(float radius, float angle, float speed);
    void radius_left(float radius, float angle, float speed);
    void forward_acc(float mm, float speed);
    void backward_acc(float mm, float speed);
    void back_buttons(float speed, std::function<bool()> first_button, std::function<bool()> second_button);
    void front_buttons(float speed, std::function<bool()> first_button, std::function<bool()> second_button);
    void wall_following(float distance_to_drive, float speed, bool automatic_distance_of_wall, float distance_of_wall, bool is_wall_on_right,
                   std::function<int()> first_sensor, 
                   std::function<int()> second_sensor, int o_kolik_je_zadni_dal);

    void orient_to_wall(bool buttom_or_right, std::function<int()> first_sensor, 
                   std::function<int()> second_sensor, int o_kolik_je_dal_zadni, float speed);

    void orient_to_wall_any_price(bool button_or_right, std::function<uint32_t()> first_sensor, 
                   std::function<uint32_t()> second_sensor, int o_kolik_je_dal_zadni, float speed);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    void initWifi(const char* ssid, const char* password);
    void print_wifi(const String& message);
    void print_wifi(const char* message);
    void printf_wifi(const char* format, ...);
    void handleWebClient();
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    rb::MotorId idLeft() const { return m_id_left; }
    rb::MotorId idRight() const { return m_id_right; }

    bool rkButton2(bool waitForRelease);
    bool rkButton1(bool waitForRelease);
    
    int16_t pctToSpeed(float pct);
    int32_t mmToTicks(float mm) const;

private:
    Motors(const Motors&) = delete;

    static int32_t scale(int32_t val);
    static int16_t pctToPower(int8_t pct);

    int32_t mmToTicks_left(float mm) const;
    int32_t mmToTicks_right(float mm) const;
    float ticksToMm(int32_t ticks) const;
    float ticksToMm_left(int32_t ticks) const;
    float ticksToMm_right(int32_t ticks) const;
    float prevod_motoru; 
    float roztec_kol; // v mm
    float left_wheel_diameter;
    float right_wheel_diameter;
    float konstanta_radius_vnejsi_kolo; // Korekční faktor pro vnější kolo při zatáčení
    float konstanta_radius_vnitrni_kolo; // Korekční faktor pro vnitřní kolo při zatáčení
    float korekce_nedotacivosti_left; // Korekce nedotáčivosti při otaceni na miste do leva
    float korekce_nedotacivosti_right; // Korekce nedotáčivosti při otaceni na miste do prava

    bool m_wifi_initialized = false;
    String m_wifi_log_buffer;
    WebServer* m_server = nullptr;

    struct DualCb {
        DualCb(dual_callback_t&& cb)
            : final_cb(std::move(cb))
            , count(0)
            , completed(false) {} // Přidání členu completed
    
        dual_callback_t final_cb;
        uint8_t count;
        bool completed; // Nový člen pro sledování dokončení
    };

    std::list<DualCb> m_dual_callbacks;
    std::mutex m_dual_callbacks_mu;

    rb::MotorId m_id_left;
    rb::MotorId m_id_right;
    float m_wheel_circumference;
    float m_wheel_circumference_left;
    float m_wheel_circumference_right;
    int32_t m_max_speed;
    bool m_polarity_switch_left;
    bool m_polarity_switch_right;
public:
    byte getButton1Pin() const { return Button1; }
    byte getButton2Pin() const { return Button2; }
private:
    byte Button1;
    byte Button2;

};

}; // namespace rk

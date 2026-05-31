#include <stdarg.h>
#include <stdio.h>
#include <Adafruit_TCS34725.h>
#include "_librk_context.h"
#include "_librk_smart_servo.h"
#include "smart_servo.h"
#include "robotka.h"
#ifdef USE_VL53L0X
#include <Adafruit_VL53L0X.h>
#include <rk_laser.cpp>
#endif
#include "RBCX.h"
#include <Adafruit_TCS34725.h>
#define TAG "robotka"

using namespace rb;
using namespace rk;
// Empty loop in case the user won't supply one
void __attribute__((weak)) loop() {
}

void rkSetup(const rkConfig& cfg) {
    gCtx.setup(cfg);
}

void rkControllerSendLog(const char* format, ...) {
    if (gCtx.prot() == nullptr) {
        ESP_LOGE(TAG, "%s: protocol not initialized!", __func__);
        return;
    }
    va_list args;
    va_start(args, format);
    gCtx.prot()->send_log(format, args);
    va_end(args);
}

void rkControllerSendLog(const std::string& text) {
    if (gCtx.prot() == nullptr) {
        ESP_LOGE(TAG, "%s: protocol not initialized!", __func__);
        return;
    }

    gCtx.prot()->send_log(text);
}

void rkControllerSend(const char* cmd, rbjson::Object* data) {
    if (gCtx.prot() == nullptr) {
        ESP_LOGE(TAG, "%s: protocol not initialized!", __func__);
        return;
    }
    gCtx.prot()->send(cmd, data);
}

void rkControllerSendMustArrive(const char* cmd, rbjson::Object* data) {
    if (gCtx.prot() == nullptr) {
        ESP_LOGE(TAG, "%s: protocol not initialized!", __func__);
        return;
    }
    gCtx.prot()->send_mustarrive(cmd, data);
}

uint32_t rkBatteryPercent() {
    return Manager::get().battery().pct();
}

uint32_t rkBatteryVoltageMv() {
    return Manager::get().battery().voltageMv();
}

int16_t rkTemperature() {
    return Manager::get().battery().temperatureC();
}

void rkMotorsSetPower(int8_t left, int8_t right) {
    gCtx.motors().setPower(left, right);
}

void rkMotorsSetPowerLeft(int8_t power) {
    gCtx.motors().setPowerById(gCtx.motors().idLeft(), power);
}

void rkMotorsSetPowerRight(int8_t power) {
    gCtx.motors().setPowerById(gCtx.motors().idRight(), power);
}

void rkMotorsSetPowerById(uint8_t id, int8_t power) {
    id -= 1;
    if (id >= (int)rb::MotorId::MAX) {
        ESP_LOGE(TAG, "%s: invalid motor id, %d is out of range <1;4>.", __func__, id + 1);
        return;
    }
    gCtx.motors().setPowerById(rb::MotorId(id), power);
}

void rkMotorsSetSpeed(int8_t left, int8_t right) {
    gCtx.motors().setSpeed(left, right);
}

void rkMotorsSetSpeedLeft(int8_t speed) {
    gCtx.motors().setSpeedById(gCtx.motors().idLeft(), speed);
}

void rkMotorsSetSpeedRight(int8_t speed) {
    gCtx.motors().setSpeedById(gCtx.motors().idRight(), speed);
}

void rkMotorsSetSpeedById(uint8_t id, int8_t speed) {
    id -= 1;
    if (id >= (int)rb::MotorId::MAX) {
        ESP_LOGE(TAG, "%s: invalid motor id, %d is out of range <1;4>.", __func__, id + 1);
        return;
    }
    gCtx.motors().setSpeedById(rb::MotorId(id), speed);
}
void rkMotorsDrive(float mmLeft, float mmRight, float speed_left, float speed_right) {
    SemaphoreHandle_t binary = xSemaphoreCreateBinary();
    gCtx.motors()
        .drive(mmLeft, mmRight, speed_left, speed_right, [=]() {
            xSemaphoreGive(binary);
        });
    xSemaphoreTake(binary, portMAX_DELAY);
    vSemaphoreDelete(binary);
}

void rkMotorsDriveLeft(float mm, uint8_t speed) {
    rkMotorsDriveById(uint8_t(gCtx.motors().idLeft()) + 1, mm, speed);
}

void rkMotorsDriveRight(float mm, uint8_t speed) {
    rkMotorsDriveById(uint8_t(gCtx.motors().idRight()) + 1, mm, speed);
}

void rkMotorsDriveById(uint8_t id, float mm, uint8_t speed) {
    id -= 1;
    if (id >= (int)rb::MotorId::MAX) {
        ESP_LOGE(TAG, "%s: invalid motor id, %d is out of range <1;4>.", __func__, id + 1);
        return;
    }

    SemaphoreHandle_t binary = xSemaphoreCreateBinary();
    gCtx.motors()
        .driveById(rb::MotorId(id), mm, speed, [=]() {
            xSemaphoreGive(binary);
        });
    xSemaphoreTake(binary, portMAX_DELAY);
    vSemaphoreDelete(binary);
}

void rkMotorsDriveAsync(float mmLeft, float mmRight, uint8_t speed_left, uint8_t speed_right, std::function<void()> callback) {
    gCtx.motors().drive(mmLeft, mmRight, speed_left, speed_right, std::move(callback));
}

void rkMotorsDriveLeftAsync(float mm, uint8_t speed, std::function<void()> callback) {
    gCtx.motors().driveById(gCtx.motors().idLeft(), mm, speed, std::move(callback));
}

void rkMotorsDriveRightAsync(float mm, uint8_t speed, std::function<void()> callback) {
    gCtx.motors().driveById(gCtx.motors().idRight(), mm, speed, std::move(callback));
}

void rkMotorsDriveByIdAsync(uint8_t id, float mm, uint8_t speed, std::function<void()> callback) {
    id -= 1;
    if (id >= (int)rb::MotorId::MAX) {
        ESP_LOGE(TAG, "%s: invalid motor id, %d is out of range <1;4>.", __func__, id + 1);
        return;
    }
    gCtx.motors().driveById(rb::MotorId(id), mm, speed, std::move(callback));
}

float rkMotorsGetPositionLeft(bool fetch) {
    return rkMotorsGetPositionById((uint8_t)gCtx.motors().idLeft() + 1, fetch);
}

float rkMotorsGetPositionRight(bool fetch) {
    return rkMotorsGetPositionById((uint8_t)gCtx.motors().idRight() + 1, fetch);
}

float rkMotorsGetPositionById(uint8_t id, bool fetch) {
    id -= 1;
    if (id >= (int)rb::MotorId::MAX) {
        ESP_LOGE(TAG, "%s: invalid motor id, %d is out of range <1;4>.", __func__, id + 1);
        return 0.f;
    }

    if(fetch) {
        SemaphoreHandle_t binary = xSemaphoreCreateBinary();
        auto& m = rb::Manager::get().motor(rb::MotorId(id));
        m.requestInfo([=](rb::Motor& m) {
            xSemaphoreGive(binary);
        });
        xSemaphoreTake(binary, portMAX_DELAY);
        vSemaphoreDelete(binary);
    }

    return gCtx.motors().position(rb::MotorId(id));
}

void rkMotorsSetPositionLeft(float positionMm) {
    rkMotorsSetPositionById((uint8_t)gCtx.motors().idLeft() + 1, positionMm);
}

void rkMotorsSetPositionRight(float positionMm) {
    rkMotorsSetPositionById((uint8_t)gCtx.motors().idRight() + 1, positionMm);
}

void rkMotorsSetPositionById(uint8_t id, float positionMm) {
    id -= 1;
    if (id >= (int)rb::MotorId::MAX) {
        ESP_LOGE(TAG, "%s: invalid motor id, %d is out of range <1;4>.", __func__, id + 1);
        return;
    }

    gCtx.motors().setPosition(rb::MotorId(id), positionMm);
}

void rkMotorsJoystick(int32_t x, int32_t y) {
    gCtx.motors().joystick(x, y);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int16_t max_rychlost(){
    return gCtx.motors().max_rychlost();
}

void forward(float mm, float speed){
    gCtx.motors().forward(mm, speed);
}
void backward(float mm, float speed){
    gCtx.motors().backward(mm, speed);
}   
void turn_on_spot_left(float angle, float speed){
    gCtx.motors().turn_on_spot_left(angle, speed);
}
void turn_on_spot_right(float angle, float speed){
    gCtx.motors().turn_on_spot_right(angle, speed);
}
void radius_right(float radius, float angle, float speed){
    gCtx.motors().radius_right(radius, angle, speed);
}
void radius_left(float radius, float angle, float speed){
    gCtx.motors().radius_left(radius, angle, speed);
}
void forward_acc(float mm, float speed){
    gCtx.motors().forward_acc(mm, speed);
}
void backward_acc(float mm, float speed){
    gCtx.motors().backward_acc(mm, speed);
}
void back_buttons(float speed, std::function<bool()> first_button, std::function<bool()> second_button){
    gCtx.motors().back_buttons(speed, first_button, second_button);
}

void front_buttons(float speed, std::function<bool()> first_button, std::function<bool()> second_button){
    gCtx.motors().front_buttons(speed, first_button, second_button);
}
void wall_following(float distance_to_drive,float speed, bool automatic_distance_of_wall ,float distance_of_wall, bool is_wall_on_right,
                   std::function<uint32_t()> first_sensor, 
                   std::function<uint32_t()> second_sensor, int o_kolik_je_zadni_dal){
    gCtx.motors().wall_following(distance_to_drive, speed, automatic_distance_of_wall ,distance_of_wall, is_wall_on_right, first_sensor, second_sensor, o_kolik_je_zadni_dal);
}
void orient_to_wall(bool buttom_or_right, std::function<uint32_t()> first_sensor, 
                   std::function<uint32_t()> second_sensor, int o_kolik_je_dal_zadni, float speed){
    gCtx.motors().orient_to_wall(buttom_or_right, first_sensor, second_sensor, o_kolik_je_dal_zadni, speed);
}

void orient_to_wall_any_price(bool button_or_right, std::function<uint32_t()> first_sensor, 
                   std::function<uint32_t()> second_sensor, int o_kolik_je_dal_zadni, float speed){
    gCtx.motors().orient_to_wall_any_price(button_or_right, first_sensor, second_sensor, o_kolik_je_dal_zadni, speed);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void rkLedRed(bool on) {
    Manager::get().leds().red(on);
}

void rkLedYellow(bool on) {
    Manager::get().leds().yellow(on);
}

void rkLedGreen(bool on) {
    Manager::get().leds().green(on);
}

void rkLedBlue(bool on) {
    Manager::get().leds().blue(on);
}

void rkLedAll(bool on) {
    auto& l = Manager::get().leds();
    l.red(on);
    l.yellow(on);
    l.green(on);
    l.blue(on);
}

void rkLedById(uint8_t id, bool on) {
    if (id == 0) {
        ESP_LOGE(TAG, "%s: invalid id %d, LEDs are indexed from 1, just like on the board (LED1, LED2...)!", __func__, id);
        return;
    } else if (id > 4) {
        ESP_LOGE(TAG, "%s: maximum LED id is 4, you are using %d!", __func__, id);
        return;
    }

    auto& l = Manager::get().leds();
    l.byId(LedId((1 << (id - 1))), on);
}

bool rkButtonIsPressed(rkButtonId id, bool waitForRelease) {
    auto& b = Manager::get().buttons();
    for (int i = 0; i < 3; ++i) {
        const bool pressed = b.byId(ButtonId(id));
        if (!pressed)
            return false;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (waitForRelease) {
        rkButtonWaitForRelease(id);
    }
    return true;
}

void rkButtonOnChangeAsync(std::function<bool(rkButtonId, bool)> callback) {
    Manager::get().buttons().onChange([callback](rb::ButtonId id, bool pressed) -> bool {
        return callback(rkButtonId(id), pressed);
    });
}

void rkButtonWaitForRelease(rkButtonId id) {
    int counter = 0;
    auto& b = Manager::get().buttons();
    while (true) {
        const bool pressed = b.byId(ButtonId(id));
        if (!pressed) {
            if (++counter > 3)
                return;
        } else {
            counter = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


uint16_t rkIrLeft() {
    return gCtx.irRead(gCtx.irChanLeft());
}

uint16_t rkIrRight() {
    return gCtx.irRead(gCtx.irChanRight());
}

uint32_t rkUltraMeasure(uint8_t id) {
    if (id == 0) {
        ESP_LOGE(TAG, "%s: invalid id %d, Ultrasounds are indexed from 1, just like on the board (U1, U2...)!", __func__, id);
        return 0;
    } else if (id > 4) {
        ESP_LOGE(TAG, "%s: maximum Ultrasound id is 4, you are using %d!", __func__, id);
        return 0;
    }

    return Manager::get().ultrasound(id - 1).measure();
}

void rkUltraMeasureAsync(uint8_t id, std::function<bool(uint32_t)> callback) {
    if (id == 0) {
        ESP_LOGE(TAG, "%s: invalid id %d, Ultrasounds are indexed from 1, just like on the board (U1, U2...)!", __func__, id);
        callback(0);
        return;
    } else if (id > 4) {
        ESP_LOGE(TAG, "%s: maximum Ultrasound id is 4, you are using %d!", __func__, id);
        callback(0);
        return;
    }

    Manager::get().ultrasound(id - 1).measureAsync(std::move(callback));
}

void rkBuzzerSet(bool on) {
    Manager::get().piezo().setState(on);
}

void rk_laser_init(const char* name,TwoWire& bus,  Adafruit_VL53L0X& lox, uint8_t   xshut_pin,uint8_t    new_address){

    return rk_laser_init_basic(name,  bus, lox, xshut_pin, new_address);
}

int rk_laser_measure(const char* name){
    return rk_laser_measure_basic(name);
}

#define MAX_COLOR_SENSORS 2

struct ColorSensor {
  const char*           name;
  Adafruit_TCS34725*    tcs;
  TwoWire*              bus;
};
static ColorSensor colorSensors[MAX_COLOR_SENSORS];
static uint8_t    colorCount = 0;

/**
 * @brief Inicializuje TCS34725 barevný senzor na zadané I2C sběrnici.
 *
 * Funkce ukládá instanci senzoru do interního pole podle jména a spouští
 * komunikaci s modulem. Každý senzor je identifikován unikátním jménem.
 *
 * @param name    Textový identifikátor senzoru (např. "front" nebo "down").
 * @param bus     Referenční I2C sběrnice (Wire nebo Wire1) pro komunikaci.
 * @param tcs     Reference na instanci Adafruit_TCS34725 pro daný senzor.
 * @return true   Pokud se senzor úspěšně inicializoval.
 * @return false  Pokud se nepodařilo spojení s modulem.
 */
bool rkColorSensorInit(const char* name, TwoWire& bus, Adafruit_TCS34725& tcs)
{
  if (colorCount >= MAX_COLOR_SENSORS) {
    Serial.println("ERROR: Max pocet color sensoru prekrocen");
    return false;
  }

  // nastav I2C sběrnici
  tcs.begin(TCS34725_ADDRESS, &bus);
  if (!tcs.begin(TCS34725_ADDRESS, &bus)) {
    Serial.print("ERROR: Nelze pripojit k color senzoru ");
    Serial.println(name);
    return false;
  }

  // ulož konfiguraci
  ColorSensor& s = colorSensors[colorCount++];
  s.name = name;
  s.tcs  = &tcs;
  s.bus  = &bus;

  return true;
}

/**
 * @brief Načte hodnoty RGB z barevného senzoru podle jeho identifikátoru.
 *
 * Funkce najde senzor v interním seznamu podle jména a zavolá getRGB, 
 * přičte kompanzní faktor a vrátí normalizované hodnoty v rozsahu 0.0–1.0.
 *
 * @param name  Textový identifikátor senzoru (stejný jako při init).
 * @param r     Ukazatel na float pro červenou složku (0.0–1.0).
 * @param g     Ukazatel na float pro zelenou složku (0.0–1.0).
 * @param b     Ukazatel na float pro modrou složku (0.0–1.0).
 * @return true   Pokud se měření úspěšně provedlo.
 * @return false  Pokud senzor není nalezen nebo měření selhalo.
 */
bool rkColorSensorGetRGB(const char* name, float* r, float* g, float* b)
{
    for (uint8_t i = 0; i < colorCount; i++) {
        ColorSensor& s = colorSensors[i];
        if (strcmp(s.name, name) == 0) {
            // Načti RGB hodnoty (0-255) přímo do poskytnutých proměnných
            s.tcs->getRGB(r, g, b);
            return true;
        }
    }
    return false;
}

void rkServosSetPosition(uint8_t id, float angleDegrees) {
    id -= 1;
    if (id >= rb::StupidServosCount) {
        ESP_LOGE(TAG, "%s: invalid id %d, must be <= %d!", __func__, id, rb::StupidServosCount);
        return;
    }
    gCtx.stupidServoSet(id, angleDegrees);
}

float rkServosGetPosition(uint8_t id) {
    id -= 1;
    if (id >= rb::StupidServosCount) {
        ESP_LOGE(TAG, "%s: invalid id %d, must be <= %d!", __func__, id, rb::StupidServosCount);
        return NAN;
    }
    return gCtx.stupidServoGet(id);
}

void rkServosDisable(uint8_t id) {
    id -= 1;
    if (id >= rb::StupidServosCount) {
        ESP_LOGE(TAG, "%s: invalid id %d, must be <= %d!", __func__, id, rb::StupidServosCount);
        return;
    }
    rb::Manager::get().stupidServo(id).disable();
}

lx16a::SmartServoBus& rkSmartServoBus(uint8_t servo_count) {
    static rk::SmartServoBusInitializer init(servo_count);
    return init.bus();
}


// Smart Servo funkce ---- 
void rkSmartServoInit(int id, int low, int high, int16_t max_diff_centideg, uint8_t  max_diff_readings) {
    rk::smart_servo::rkSmartServoInit(id, low, high, max_diff_centideg, max_diff_readings);
}

void rkSmartServoMove(int id, int angle, int speed) {
    rk::smart_servo::rkSmartServoMove(id,angle,speed);
}

void rkSmartServoSoftMove(int id, int angle, int speed) {
    rk::smart_servo::rkSmartServoSoftMove(id, angle, speed);
}

byte rkSmartServosPosicion(int id) {
    return rk::smart_servo::rkSmartServosPosicion(id);
}


void printf_wifi(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int len = vsnprintf(nullptr, 0, format, args);
    va_end(args);

    std::string buf;
    buf.resize(len + 1);
    va_start(args, format);
    vsnprintf(&buf[0], buf.size(), format, args);
    va_end(args);

    gCtx.motors().printf_wifi(buf.c_str());
}
void handleWebClients() {
    gCtx.motors().handleWebClient();
}

void wifi_control_wasd() {
    
    // Hlavní smyčka pro obsluhu webových klientů
    bool still = true;
    while(still) {
        still = rk::Wifi::handleWebClients();
        delay(50);
    }
}

void wifi_control_terminal() {
    while(true){
        rk::Wifi::handleWebClients_terminal();
        delay(50);
    }
}
// Pomocná funkce pro parsování parametrů
static bool parseParams(const String& params, float* out, int count) {
    int idx = 0, last = 0;
    for (int i = 0; i < count; ++i) {
        idx = params.indexOf(',', last);
        String val = (idx == -1) ? params.substring(last) : params.substring(last, idx);
        val.trim();
        if (!val.length()) return false;
        out[i] = val.toFloat();
        last = idx + 1;
        if (idx == -1 && i < count - 1) return false;
    }
    return true;
}
// Funkce pro zpracování příkazů
static void processCommand(const String &cmd) {
    // FORWARD - forward(mm, speed)
    if (cmd.startsWith("forward(")) {
        float params[2];
        int start = cmd.indexOf('(') + 1;
        int end = cmd.indexOf(')');
        if (start < 1 || end < 0) {
            Serial.println("Chybný formát forward");
            return;
        }
        String paramStr = cmd.substring(start, end);
        if (!parseParams(paramStr, params, 2)) {
            Serial.println("Chyba v parametrech forward");
            return;
        }
        forward(params[0], params[1]);
        Serial.println("forward zavoláno");
        return;
    }
    
    // FORWARD_ACC - forward_acc(mm, speed)
    if (cmd.startsWith("forward_acc(")) {
        float params[2];
        int start = cmd.indexOf('(') + 1;
        int end = cmd.indexOf(')');
        if (start < 1 || end < 0) {
            Serial.println("Chybný formát forward_acc");
            return;
        }
        String paramStr = cmd.substring(start, end);
        if (!parseParams(paramStr, params, 2)) {
            Serial.println("Chyba v parametrech forward_acc");
            return;
        }
        forward_acc(params[0], params[1]);
        Serial.println("forward_acc zavoláno");
        return;
    }
    
    // BACKWARD - backward(mm, speed)
    if (cmd.startsWith("backward(")) {
        float params[2];
        int start = cmd.indexOf('(') + 1;
        int end = cmd.indexOf(')');
        if (start < 1 || end < 0) {
            Serial.println("Chybný formát backward");
            return;
        }
        String paramStr = cmd.substring(start, end);
        if (!parseParams(paramStr, params, 2)) {
            Serial.println("Chyba v parametrech backward");
            return;
        }
        backward(params[0], params[1]);
        Serial.println("backward zavoláno");
        return;
    }
    
    // BACKWARD_ACC - backward_acc(mm, speed)
    if (cmd.startsWith("backward_acc(")) {
        float params[2];
        int start = cmd.indexOf('(') + 1;
        int end = cmd.indexOf(')');
        if (start < 1 || end < 0) {
            Serial.println("Chybný formát backward_acc");
            return;
        }
        String paramStr = cmd.substring(start, end);
        if (!parseParams(paramStr, params, 2)) {
            Serial.println("Chyba v parametrech backward_acc");
            return;
        }
        backward_acc(params[0], params[1]);
        Serial.println("backward_acc zavoláno");
        return;
    }
    
    // TURN_ON_SPOT_LEFT - turn_on_spot_left(angle, speed)
    if (cmd.startsWith("turn_on_spot_left(")) {
        float params[2];
        int start = cmd.indexOf('(') + 1;
        int end = cmd.indexOf(')');
        if (start < 1 || end < 0) {
            Serial.println("Chybný formát turn_on_spot_left");
            return;
        }
        String paramStr = cmd.substring(start, end);
        if (!parseParams(paramStr, params, 2)) {
            Serial.println("Chyba v parametrech turn_on_spot_left");
            return;
        }
        turn_on_spot_left(params[0], params[1]);
        Serial.println("turn_on_spot_left zavoláno");
        return;
    }
    
    // TURN_ON_SPOT_RIGHT - turn_on_spot_right(angle, speed)
    if (cmd.startsWith("turn_on_spot_right(")) {
        float params[2];
        int start = cmd.indexOf('(') + 1;
        int end = cmd.indexOf(')');
        if (start < 1 || end < 0) {
            Serial.println("Chybný formát turn_on_spot_right");
            return;
        }
        String paramStr = cmd.substring(start, end);
        if (!parseParams(paramStr, params, 2)) {
            Serial.println("Chyba v parametrech turn_on_spot_right");
            return;
        }
        turn_on_spot_right(params[0], params[1]);
        Serial.println("turn_on_spot_right zavoláno");
        return;
    }
    
    // RADIUS_LEFT - radius_left(radius, angle, speed)
    if (cmd.startsWith("radius_left(")) {
        float params[3];
        int start = cmd.indexOf('(') + 1;
        int end = cmd.indexOf(')');
        if (start < 1 || end < 0) {
            Serial.println("Chybný formát radius_left");
            return;
        }
        String paramStr = cmd.substring(start, end);
        if (!parseParams(paramStr, params, 3)) {
            Serial.println("Chyba v parametrech radius_left");
            return;
        }
        radius_left(params[0], params[1], params[2]);
        Serial.println("radius_left zavoláno");
        return;
    }
    
    // RADIUS_RIGHT - radius_right(radius, angle, speed)
    if (cmd.startsWith("radius_right(")) {
        float params[3];
        int start = cmd.indexOf('(') + 1;
        int end = cmd.indexOf(')');
        if (start < 1 || end < 0) {
            Serial.println("Chybný formát radius_right");
            return;
        }
        String paramStr = cmd.substring(start, end);
        if (!parseParams(paramStr, params, 3)) {
            Serial.println("Chyba v parametrech radius_right");
            return;
        }
        radius_right(params[0], params[1], params[2]);
        Serial.println("radius_right zavoláno");
        return;
    }
    
    // BACK_BUTTONS - back_buttons(speed)
    if (cmd.startsWith("back_buttons(")) {
        float params[1];
        int start = cmd.indexOf('(') + 1;
        int end = cmd.indexOf(')');
        if (start < 1 || end < 0) {
            Serial.println("Chybný formát back_buttons");
            return;
        }
        String paramStr = cmd.substring(start, end);
        if (!parseParams(paramStr, params, 1)) {
            Serial.println("Chyba v parametrech back_buttons");
            return;
        }
        back_buttons(params[0], []{return false;}, []{return false;});
        Serial.println("back_buttons zavoláno");
        return;
    }

    // MAX_RYCHLOST - max_rychlost()
    if (cmd == "max_rychlost()") {
        int16_t rychlost = max_rychlost();
        Serial.printf("Max rychlost: %d\n", rychlost);
        return;
    }

    // STOP - stop()
    if (cmd == "stop()") {
        rkMotorsSetPower(0, 0);
        Serial.println("stop zavoláno");
        return;
    }

    // SET_SPEED - set_speed(left, right)
    if (cmd.startsWith("set_speed(")) {
        float params[2];
        int start = cmd.indexOf('(') + 1;
        int end = cmd.indexOf(')');
        if (start < 1 || end < 0) {
            Serial.println("Chybný formát set_speed");
            return;
        }
        String paramStr = cmd.substring(start, end);
        if (!parseParams(paramStr, params, 2)) {
            Serial.println("Chyba v parametrech set_speed");
            return;
        }
        rkMotorsSetSpeed(params[0], params[1]);
        Serial.println("set_speed zavoláno");
        return;
    }

    // SET_POWER - set_power(left, right)
    if (cmd.startsWith("set_power(")) {
        float params[2];
        int start = cmd.indexOf('(') + 1;
        int end = cmd.indexOf(')');
        if (start < 1 || end < 0) {
            Serial.println("Chybný formát set_power");
            return;
        }
        String paramStr = cmd.substring(start, end);
        if (!parseParams(paramStr, params, 2)) {
            Serial.println("Chyba v parametrech set_power");
            return;
        }
        rkMotorsSetPower(params[0], params[1]);
        Serial.println("set_power zavoláno");
        return;
    }

    // SMART SERVO INIT - rkSmartServoInit(id, low, high)
    if (cmd.startsWith("servo_init(")) {
        float params[3] = {0, 0, 240}; // id, low, high (default values)
        int start = cmd.indexOf('(') + 1;
        int end = cmd.indexOf(')');
        if (start < 1 || end < 0) {
            Serial.println("Chybný formát servo_init");
            return;
        }
        String paramStr = cmd.substring(start, end);
        
        // Zjisti počet parametrů
        int paramCount = 1;
        for (int i = 0; i < paramStr.length(); i++) {
            if (paramStr.charAt(i) == ',') paramCount++;
        }
        
        if (!parseParams(paramStr, params, paramCount)) {
            Serial.println("Chyba v parametrech servo_init");
            return;
        }
        
        int id = (int)params[0];
        int low = (paramCount > 1) ? (int)params[1] : 0;
        int high = (paramCount > 2) ? (int)params[2] : 240;
        
        rkSmartServoInit(id, low, high);
        Serial.println("servo_init zavoláno");
        return;
    }
    
    // SMART SERVO MOVE - rkSmartServoMove(id, angle, speed)
    if (cmd.startsWith("servo_move(")) {
        float params[3] = {0, 0, 200}; // id, angle, speed (default values)
        int start = cmd.indexOf('(') + 1;
        int end = cmd.indexOf(')');
        if (start < 1 || end < 0) {
            Serial.println("Chybný formát servo_move");
            return;
        }
        String paramStr = cmd.substring(start, end);
        
        // Zjisti počet parametrů
        int paramCount = 1;
        for (int i = 0; i < paramStr.length(); i++) {
            if (paramStr.charAt(i) == ',') paramCount++;
        }
        
        if (!parseParams(paramStr, params, paramCount)) {
            Serial.println("Chyba v parametrech servo_move");
            return;
        }
        
        int id = (int)params[0];
        int angle = (int)params[1];
        int speed = (paramCount > 2) ? (int)params[2] : 200;
        
        rkSmartServoMove(id, angle, speed);
        Serial.println("servo_move zavoláno");
        return;
    }
    
    // SMART SERVO SOFT MOVE - rkSmartServoSoftMove(id, angle, speed)
    if (cmd.startsWith("servo_soft_move(")) {
        float params[3] = {0, 0, 200}; // id, angle, speed (default values)
        int start = cmd.indexOf('(') + 1;
        int end = cmd.indexOf(')');
        if (start < 1 || end < 0) {
            Serial.println("Chybný formát servo_soft_move");
            return;
        }
        String paramStr = cmd.substring(start, end);
        
        // Zjisti počet parametrů
        int paramCount = 1;
        for (int i = 0; i < paramStr.length(); i++) {
            if (paramStr.charAt(i) == ',') paramCount++;
        }
        
        if (!parseParams(paramStr, params, paramCount)) {
            Serial.println("Chyba v parametrech servo_soft_move");
            return;
        }
        
        int id = (int)params[0];
        int angle = (int)params[1];
        int speed = (paramCount > 2) ? (int)params[2] : 200;
        
        rkSmartServoSoftMove(id, angle, speed);
        Serial.println("servo_soft_move zavoláno");
        return;
    }
    
    // SMART SERVO POSITION - rkSmartServoPosition(id)
    if (cmd.startsWith("servo_position(")) {
        float params[1];
        int start = cmd.indexOf('(') + 1;
        int end = cmd.indexOf(')');
        if (start < 1 || end < 0) {
            Serial.println("Chybný formát servo_position");
            return;
        }
        String paramStr = cmd.substring(start, end);
        if (!parseParams(paramStr, params, 1)) {
            Serial.println("Chyba v parametrech servo_position");
            return;
        }
        
        int id = (int)params[0];
        byte position = rkSmartServosPosicion(id);
        Serial.printf("Smart Servo %d pozice: %d°\n", id, position);
        return;
    }
    
    
    Serial.println("Neznámý příkaz");
}

void rkSerialTerminal() {
    // Kontrola, zda je Serial inicializován
    if (!Serial) {
        ESP_LOGE(TAG, "Serial není inicializován! Volej Serial.begin(115200) v setup()");
        return;
    }
    
    Serial.println("=== ROBOTKA SERIAL TERMINAL ===");
    Serial.println("Dostupné příkazy:");
    Serial.println("=== POHYB ROBOTA ===");
    Serial.println("forward(mm, speed)           - např. forward(1000, 50)");
    Serial.println("forward_acc(mm, speed)       - např. forward_acc(800, 50)");
    Serial.println("backward(mm, speed)          - např. backward(800, 40)");
    Serial.println("backward_acc(mm, speed)      - např. backward_acc(600, 40)");
    Serial.println("turn_on_spot_left(angle, speed) - např. turn_on_spot_left(90, 40)");
    Serial.println("turn_on_spot_right(angle, speed)- např. turn_on_spot_right(90, 40)");
    Serial.println("radius_left(radius, angle, speed) - např. radius_left(200, 90, 40)");
    Serial.println("radius_right(radius, angle, speed)- např. radius_right(200, 90, 40)");
    Serial.println("back_buttons(speed)          - např. back_buttons(30)");
    Serial.println("max_rychlost()               - změří maximální rychlost motorů");
    Serial.println("stop()                       - okamžité zastavení motorů");
    Serial.println("set_speed(left, right)       - nastaví rychlost motorů v % (-100 až 100)");
    Serial.println("set_power(left, right)       - nastaví výkon motorů v % (-100 až 100)");
    Serial.println("=== SMART SERVA ===");
    Serial.println("servo_init(id, [low, high])  - např. servo_init(1) nebo servo_init(1, 0, 180)");
    Serial.println("servo_move(id, angle, [speed]) - např. servo_move(1, 90) nebo servo_move(1, 90, 300)");
    Serial.println("servo_soft_move(id, angle, [speed]) - např. servo_soft_move(1, 90, 150)");
    Serial.println("servo_position(id)           - např. servo_position(1)");
    Serial.println("Zadej příkaz:");
    
    while(true) {
        if (Serial.available()) {
            String line = Serial.readStringUntil('\n');
            line.trim();
            if (line.length() > 0) {
                processCommand(line);
            }
        }
        delay(15);
    }
}

// UART proměnné
enum RxState { WAIT_SYNC0, WAIT_SYNC1, READ_PAYLOAD };

static HardwareSerial* uartSerial = &Serial1;
static bool uartInitialized = false;

bool rkUartInit(int baudRate, int rxPin, int txPin) {
    if (uartInitialized) {
        ESP_LOGW(TAG, "UART již byl inicializován");
        return true;
    }
    
    uartSerial->begin(baudRate, SERIAL_8N1, rxPin, txPin);
    uartInitialized = true;
    ESP_LOGI(TAG, "UART inicializován: RX=%d, TX=%d, baud=%d", rxPin, txPin, baudRate);
    return true;
}

bool rkUartReceive_blocking(void* msg, size_t msgSize, uint32_t timeoutMs) {
    if (!uartInitialized || msg == nullptr) {
        return false;
    }
    
    const uint8_t SYNC0 = 0xAA;
    const uint8_t SYNC1 = 0x55;
    
    RxState state = WAIT_SYNC0;
    
    uint8_t* buffer = (uint8_t*)msg;
    size_t bytesRead = 0;
    uint32_t startTime = millis();
    
    while ((millis() - startTime) < timeoutMs) {
        if (uartSerial->available()) {
            uint8_t c = uartSerial->read();
            
            switch (state) {
                case WAIT_SYNC0:
                    if (c == SYNC0) state = WAIT_SYNC1;
                    break;
                    
                case WAIT_SYNC1:
                    if (c == SYNC1) {
                        state = READ_PAYLOAD;
                        bytesRead = 0;
                    } else {
                        state = (c == SYNC0) ? WAIT_SYNC1 : WAIT_SYNC0;
                    }
                    break;
                    
                case READ_PAYLOAD:
                    buffer[bytesRead++] = c;
                    if (bytesRead >= msgSize) {
                        return true; // Úspěšně přijato
                    }
                    break;
            }
        }
        delay(1); // Malé zpoždění pro snížení zátěže CPU
    }
    
    return false; // Timeout
}

bool rkUartReceive(void* msg, size_t msgSize) {
    if (!uartInitialized || msg == nullptr) {
        return false;
    }
    
    const uint8_t SYNC0 = 0xAA;
    const uint8_t SYNC1 = 0x55;
    
    static RxState state = WAIT_SYNC0;
    static size_t bytesRead = 0;
    static uint8_t* buffer = (uint8_t*)msg;
    
    while (uartSerial->available()) {
        uint8_t c = uartSerial->read();
        
        switch (state) {
            case WAIT_SYNC0:
                if (c == SYNC0) state = WAIT_SYNC1;
                break;
                
            case WAIT_SYNC1:
                if (c == SYNC1) {
                    state = READ_PAYLOAD;
                    bytesRead = 0;
                } else {
                    state = (c == SYNC0) ? WAIT_SYNC1 : WAIT_SYNC0;
                }
                break;
                
            case READ_PAYLOAD:
                buffer[bytesRead++] = c;
                if (bytesRead >= msgSize) {
                    state = WAIT_SYNC0;
                    return true;
                }
                break;
        }
    }
    
    return false;
}

void rkUartSend(const void* msg, size_t msgSize) {
    if (!uartInitialized || msg == nullptr) {
        return;
    }
    
    const uint8_t SYNC0 = 0xAA;
    const uint8_t SYNC1 = 0x55;
    
    // Odeslání synchronizačních bytů
    uartSerial->write(SYNC0);
    uartSerial->write(SYNC1);
    
    // Odeslání dat
    size_t bytesWritten = uartSerial->write((const uint8_t*)msg, msgSize);
    printf("Odesláno %d bytů přes UART\n", bytesWritten);
}

bool rkButton1(bool waitForRelease) {
    return gCtx.motors().rkButton1(waitForRelease);
}

bool rkButton2(bool waitForRelease) {
    return gCtx.motors().rkButton2(waitForRelease);
}

static float mpu_offset_x = 0.0f;
static float mpu_offset_y = 0.0f;

void rkMpuInit() {
    auto& mpu = rb::Manager::get().mpu();
    mpu.init();
    mpu.sendStart();

    // Počkáme chvíli, než začnou chodit první stabilní data ze senzoru
    delay(1000);

    // Vymažeme stará kalibrační data (nastaví offset na 0)
    mpu.clearCalibrationData();
    delay(100); // Necháme do knihovny natéct čistá data (bez offsetu)

    // Zkalibrujeme! Uloží aktuální odchylku (chybu) jako offset, který se bude nově odečítat.
    // DŮLEŽITÉ: ROBOT V TUTO CHVÍLI MUSÍ BÝT V ABSOLUTNÍM KLIDU!
    mpu.setCalibrationData();
    
    // Vynulujeme celkový úhel osy Z, aby nám hezky začínal od nuly
    mpu.resetAngleZ();
    mpu_offset_x = 0.0f;
    mpu_offset_y = 0.0f;
}

float rkMpuGetAngleZ() {
    return rb::Manager::get().mpu().getAngleZ();
}

float rkMpuGetAngleX() {
    return rb::Manager::get().mpu().getAngleX() - mpu_offset_x;
}

float rkMpuGetAngleY() {
    return rb::Manager::get().mpu().getAngleY() - mpu_offset_y;
}

void rkMpuResetZ() {
    rb::Manager::get().mpu().resetAngleZ();
}

void rkMpuResetX() {
    // Pro zresetování uložíme aktuální absolutní úhel jako offset, 
    // který se pak v GetAngle odečítá
    mpu_offset_x = rb::Manager::get().mpu().getAngleX();
}

void rkMpuResetY() {
    mpu_offset_y = rb::Manager::get().mpu().getAngleY();
}

void rkMpuResetAll() {
    rkMpuResetZ();
    rkMpuResetX();
    rkMpuResetY();
}

#include <WiFi.h>
#include <WiFiUdp.h>
#include <iostream>
#include "wifi_control.h"

namespace rk {

WiFiUDP udp;
unsigned int localPort = 1234; // Port pro UDP server

void Wifi::setupWiFiControl(const rkConfig& cfg) {
    const char* ap_ssid = cfg.wifi_ssid;      // Název WiFi sítě
    const char* ap_password = cfg.wifi_password;  // Heslo WiFi sítě (minimálně 8 znaků)
    
    // Vytvoření WiFi access pointu
    printf("Creating WiFi Access Point...\n");
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid, ap_password);

    IPAddress myIP = WiFi.softAPIP();
    printf("AP IP address: ");
    printf("%s\n", myIP.toString().c_str());
    
    // Spuštění UDP serveru
    udp.begin(localPort);
    printf("UDP server started on port %d\n", localPort);
    printf("Ready for connections!\n");
    printf("To connect:\n");
    printf("1. Connect to WiFi network '%s'\n", ap_ssid);
    printf("2. Use password: %s\n", ap_password);
    printf("3. Robot IP address is: %s\n", myIP.toString().c_str());
}
byte cout=0;

bool Wifi::handleWebClients() {
    int packetSize = udp.parsePacket();
    
    if (packetSize) {
        // Přijali jsme data
        char incomingPacket[255];
        int len = udp.read(incomingPacket, 255);
        if (len > 0) {
            incomingPacket[len] = 0;  // Ukončit string
        }
        
        printf("Received command: %s\n", incomingPacket);
        
        // Zpracování příkazu - ZDE BUDEŠ OVLÁDAT MOTORY

        if (strcmp(incomingPacket, "OFF") == 0) {
            // VYPNI MOTORY - přidej svůj kód  
            rkMotorsSetSpeed(0, 0);
            printf("Konec wasd\n");
            return false;
            
        } else if (strcmp(incomingPacket, "FORWARD") == 0) {
            rkMotorsSetSpeed(60, 60);
            
        } else if (strcmp(incomingPacket, "BACKWARD") == 0) {
            rkMotorsSetSpeed(-60, -60);
            
        } else if (strcmp(incomingPacket, "LEFT") == 0) {
            rkMotorsSetSpeed(-20, 20);
            
        } else if (strcmp(incomingPacket, "RIGHT") == 0) {
            rkMotorsSetSpeed(20, -20);

        }else if(strcmp(incomingPacket, "FUNC1")){
            printf("Function 1 executed!\n");
            // Zde přidej kód pro funkci 1

        }else if(strcmp(incomingPacket, "FUNC2")){
            printf("Function 2 executed!\n");
            // Zde přidej kód pro funkci 2
        
        }else if(strcmp(incomingPacket, "FUNC3")){
            printf("Function 3 executed!\n");
            // Zde přidej kód pro funkci 3
        }else if(strcmp(incomingPacket, "FUNC4")){
            printf("Function 4 executed!\n");
            // Zde přidej kód pro funkci 4
        }
        else if(strcmp(incomingPacket, "FUNC5")){
            printf("Function 5 executed!\n");
            // Zde přidej kód pro funkci 5
        }
        else if(strcmp(incomingPacket, "FUNC6")){
            printf("Function 6 executed!\n");
            // Zde přidej kód pro funkci 6
        }
        else if(strcmp(incomingPacket, "FUNC7")){
            printf("Function 7 executed!\n");
            // Zde přidej kód pro funkci 7
        }else if(strcmp(incomingPacket, "FUNC8")){
            printf("Function 8 executed!\n");
            // Zde přidej kód pro funkci 8
        }
        else if(strcmp(incomingPacket, "FUNC9")){
            printf("Function 9 executed!\n");
            // Zde přidej kód pro funkci 9
        }
        else {
            printf("Unknown command received: %s\n", incomingPacket);
        }
        
    }
    else{
        // Žádná data
        printf("No data received.\n");
        cout++;
        printf("Cout: %d\n", cout);
        if(cout >= 3){
            rkMotorsSetPower(0, 0); // Zastavit motory, pokud není příkaz
            printf("Motors stopped due to no commands.\n");
            cout=0;
        }
    }
    
    delay(10);  // Krátké zpoždění
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void Wifi::handleWebClients_terminal() {
    int packetSize = udp.parsePacket();
    
    if (packetSize) {
        // Přijali jsme data
        char incomingPacket[255];
        int len = udp.read(incomingPacket, 255);
        if (len > 0) {
            incomingPacket[len] = 0;  // Ukončit string
        }
        
        printf("Received command: %s\n", incomingPacket);

        int param1 = 0, param2 = 0, param3 = 0, param4 = 0;
        float fparam1 = 0;
        char strParam[50];

        // Příkazy pro motory
        if (sscanf(incomingPacket, "forward(%d,%d)", &param1, &param2) == 2) {
            printf("🚀 FORWARD: délka=%d, rychlost=%d\n", param1, param2);
            forward(param1, param2);
            
        } else if (sscanf(incomingPacket, "backward(%d,%d)", &param1, &param2) == 2) {
            printf("🔙 BACKWARD: délka=%d, rychlost=%d\n", param1, param2);
            backward(param1, param2);
            
        } else if (sscanf(incomingPacket, "turn_left(%d,%d)", &param1, &param2) == 2) {
            printf("⬅️ TURN LEFT: úhel=%d, rychlost=%d\n", param1, param2);
            turn_on_spot_left(param1, param2);
            
        } else if (sscanf(incomingPacket, "turn_right(%d,%d)", &param1, &param2) == 2) {
            printf("➡️ TURN RIGHT: úhel=%d, rychlost=%d\n", param1, param2);
            turn_on_spot_right(param1, param2);
            
        } else if (sscanf(incomingPacket, "radius_left(%d,%d,%d)", &param1, &param2, &param3) == 3) {
            printf("🔄 RADIUS LEFT: poloměr=%d, úhel=%d, rychlost=%d\n", param1, param2, param3);
            radius_left(param1, param2, param3);
            
        } else if (sscanf(incomingPacket, "radius_right(%d,%d,%d)", &param1, &param2, &param3) == 3) {
            printf("🔄 RADIUS RIGHT: poloměr=%d, úhel=%d, rychlost=%d\n", param1, param2, param3);
            radius_right(param1, param2, param3);
            
        } else if (sscanf(incomingPacket, "forward_acc(%d,%d)", &param1, &param2) == 2) {
            printf("🚀 FORWARD ACC: délka=%d, rychlost=%d\n", param1, param2);
            forward_acc(param1, param2);
            
        } else if (sscanf(incomingPacket, "backward_acc(%d,%d)", &param1, &param2) == 2) {
            printf("🔙 BACKWARD ACC: délka=%d, rychlost=%d\n", param1, param2);
            backward_acc(param1, param2);
            
        } else if (sscanf(incomingPacket, "back_buttons(%d)", &param1) == 1) {
            printf("🔙 BACK BUTTONS: rychlost=%d\n", param1);
            back_buttons(param1, []{return false;}, []{return false;});
            
        } else if (sscanf(incomingPacket, "power(%d,%d)", &param1, &param2) == 2) {
            printf("⚡ POWER: levý=%d, pravý=%d\n", param1, param2);
            rkMotorsSetPower(param1, param2);
            
        } else if (sscanf(incomingPacket, "speed(%d,%d)", &param1, &param2) == 2) {
            printf("⚡ SPEED: levý=%d, pravý=%d\n", param1, param2);
            rkMotorsSetSpeed(param1, param2);
            
        } else if (sscanf(incomingPacket, "drive(%d,%d,%d,%d)", &param1, &param2, &param3, &param4) == 4) {
            printf("🎯 DRIVE: levý=%dmm, pravý=%dmm, rychlost_levý=%d, rychlost_pravý=%d\n", param1, param2, param3, param4);
            rkMotorsDrive(param1, param2, param3, param4);

        // Příkazy pro serva
        } else if (sscanf(incomingPacket, "servo(%d,%f)", &param1, &fparam1) == 2) {
            printf("🔄 SERVO: id=%d, úhel=%.2f\n", param1, fparam1);
            rkServosSetPosition(param1, fparam1);
            
        } else if (sscanf(incomingPacket, "servo_off(%d)", &param1) == 1) {
            printf("🔄 SERVO OFF: id=%d\n", param1);
            rkServosDisable(param1);
            
        } else if (sscanf(incomingPacket, "smart_servo(%d,%d)", &param1, &param2) == 2) {
            printf("🤖 SMART SERVO: id=%d, pozice=%d\n", param1, param2);
            // Poznámka: Pro chytrá serva je potřeba získat instanci bus a nastavit pozici
            // lx16a::SmartServoBus& bus = rkSmartServoBus(0);
            // bus.setPosition(param1, param2, 1000); // ID, pozice, čas v ms

        // Příkazy pro LEDky
        } else if (sscanf(incomingPacket, "led(%d,%d)", &param1, &param2) == 2) {
            printf("💡 LED: id=%d, stav=%d\n", param1, param2);
            rkLedById(param1, param2 != 0);
            
        } else if (strcmp(incomingPacket, "led_red(1)") == 0) {
            printf("💡 LED RED ON\n");
            rkLedRed(true);
            
        } else if (strcmp(incomingPacket, "led_red(0)") == 0) {
            printf("💡 LED RED OFF\n");
            rkLedRed(false);
            
        } else if (strcmp(incomingPacket, "led_yellow(1)") == 0) {
            printf("💡 LED YELLOW ON\n");
            rkLedYellow(true);
            
        } else if (strcmp(incomingPacket, "led_yellow(0)") == 0) {
            printf("💡 LED YELLOW OFF\n");
            rkLedYellow(false);
            
        } else if (strcmp(incomingPacket, "led_green(1)") == 0) {
            printf("💡 LED GREEN ON\n");
            rkLedGreen(true);
            
        } else if (strcmp(incomingPacket, "led_green(0)") == 0) {
            printf("💡 LED GREEN OFF\n");
            rkLedGreen(false);
            
        } else if (strcmp(incomingPacket, "led_blue(1)") == 0) {
            printf("💡 LED BLUE ON\n");
            rkLedBlue(true);
            
        } else if (strcmp(incomingPacket, "led_blue(0)") == 0) {
            printf("💡 LED BLUE OFF\n");
            rkLedBlue(false);
            
        } else if (strcmp(incomingPacket, "led_all(1)") == 0) {
            printf("💡 LED ALL ON\n");
            rkLedAll(true);
            
        } else if (strcmp(incomingPacket, "led_all(0)") == 0) {
            printf("💡 LED ALL OFF\n");
            rkLedAll(false);

        // Příkazy pro ultrazvuk
        } else if (sscanf(incomingPacket, "ultra(%d)", &param1) == 1) {
            printf("📏 ULTRASOUND: id=%d\n", param1);
            uint32_t distance = rkUltraMeasure(param1);
            printf("📏 Vzdálenost: %u mm\n", distance);

        // Příkazy pro bzučák
        } else if (strcmp(incomingPacket, "buzzer(1)") == 0) {
            printf("🔊 BUZZER ON\n");
            rkBuzzerSet(true);
            
        } else if (strcmp(incomingPacket, "buzzer(0)") == 0) {
            printf("🔊 BUZZER OFF\n");
            rkBuzzerSet(false);

        // Příkazy pro barevný senzor
        } else if (sscanf(incomingPacket, "color(%49[^)])", strParam) == 1) {
            printf("🎨 COLOR: senzor=%s\n", strParam);
            float r, g, b;
            if (rkColorSensorGetRGB(strParam, &r, &g, &b)) {
                printf("🎨 RGB: R=%.2f, G=%.2f, B=%.2f\n", r, g, b);
                // Odeslání zpět přes UDP
                char response[100];
                snprintf(response, sizeof(response), "RGB: R=%.2f, G=%.2f, B=%.2f", r, g, b);
                udp.beginPacket(udp.remoteIP(), udp.remotePort());
                udp.write((uint8_t*)response, strlen(response));
                udp.endPacket();
            } else {
                printf("🎨 Chyba čtení barevného senzoru\n");
            }

        // Příkazy pro laser (VL53L0X)
        } else if (sscanf(incomingPacket, "laser(%49[^)])", strParam) == 1) {
            printf("🎯 LASER: senzor=%s\n", strParam);
            #ifdef USE_VL53L0X
            int distance = rk_laser_measure(strParam);
            if (distance >= 0) {
                printf("🎯 Vzdálenost: %d mm\n", distance);
                // Odeslání zpět přes UDP
                char response[50];
                snprintf(response, sizeof(response), "Vzdálenost: %d mm", distance);
                udp.beginPacket(udp.remoteIP(), udp.remotePort());
                udp.write((uint8_t*)response, strlen(response));
                udp.endPacket();
            } else {
                printf("🎯 Chyba měření laserem\n");
            }
            #else
            printf("🎯 Laser senzor není podporován (USE_VL53L0X není definováno)\n");
            #endif

        // Příkazy pro baterii a teplotu
        } else if (strcmp(incomingPacket, "battery()") == 0) {
            uint32_t voltage = rkBatteryVoltageMv();
            uint32_t percent = rkBatteryPercent();
            int16_t temp = rkTemperature();
            printf("🔋 BATERIE: %u mV, %u%%, Teplota: %d°C\n", voltage, percent, temp);
            
            char response[100];
            snprintf(response, sizeof(response), "Baterie: %u mV, %u%%, Teplota: %d°C", voltage, percent, temp);
            udp.beginPacket(udp.remoteIP(), udp.remotePort());
            udp.write((uint8_t*)response, strlen(response));
            udp.endPacket();

        // Příkazy pro IR senzory
        } else if (strcmp(incomingPacket, "ir_left()") == 0) {
            uint16_t ir_value = rkIrLeft();
            printf("📟 IR LEFT: %u\n", ir_value);
            
        } else if (strcmp(incomingPacket, "ir_right()") == 0) {
            uint16_t ir_value = rkIrRight();
            printf("📟 IR RIGHT: %u\n", ir_value);

        // Obecné příkazy
        } else if (strcmp(incomingPacket, "on()") == 0) {
            printf("🟢 ZAPNU MOTORY\n");
            rkMotorsSetPower(50, 50);
            
        } else if (strcmp(incomingPacket, "off()") == 0) {
            printf("🔴 VYPNU MOTORY\n");
            rkMotorsSetPower(0, 0);
            
        } else if (strcmp(incomingPacket, "stop") == 0) {
            printf("⏹️ OKAMŽITÉ ZASTAVENÍ\n");
            rkMotorsSetPower(0, 0);
            
        } else if (strcmp(incomingPacket, "help()") == 0) {
            printf("🤖 POKROČILÝ OVLADAČ ROBOTA\n");
            printf("==============================\n");
            printf("Dostupné příkazy:\n");
            printf("🎯 POHYB:\n");
            printf("  forward(délka, rychlost)     - vpřed\n");
            printf("  backward(délka, rychlost)    - vzad\n");
            printf("  turn_left(úhel, rychlost)    - otáčení vlevo\n");
            printf("  turn_right(úhel, rychlost)   - otáčení vpravo\n");
            printf("  radius_left(r,úhel,rychlost) - zatáčka vlevo\n");
            printf("  radius_right(r,úhel,rychlost)- zatáčka vpravo\n");
            printf("  power(levý, pravý)           - přímý výkon\n");
            printf("  speed(levý, pravý)           - nastavení rychlosti\n");
            printf("  drive(lmm,rmm,lrychl,prychl) - řízení pojezdu\n");
            printf("\n");
            printf("🔧 SERVA:\n");
            printf("  servo(id,úhel)               - hloupé servo\n");
            printf("  servo_off(id)                - vypnutí serva\n");
            printf("  smart_servo(id,pozice)       - chytré servo\n");
            printf("\n");
            printf("💡 LED:\n");
            printf("  led(id,stav)                 - LED podle ID\n");
            printf("  led_red(1/0)                 - červená LED\n");
            printf("  led_yellow(1/0)              - žlutá LED\n");
            printf("  led_green(1/0)               - zelená LED\n");
            printf("  led_blue(1/0)                - modrá LED\n");
            printf("  led_all(1/0)                 - všechny LED\n");
            printf("\n");
            printf("📊 SENZORY:\n");
            printf("  ultra(id)                    - ultrazvuk\n");
            printf("  color(název)                 - barevný senzor\n");
            printf("  laser(název)                 - laserový dálkoměr\n");
            printf("  ir_left()                    - IR senzor vlevo\n");
            printf("  ir_right()                   - IR senzor vpravo\n");
            printf("  battery()                    - stav baterie\n");
            printf("\n");
            printf("🔊 BZUČÁK:\n");
            printf("  buzzer(1/0)                  - zap/vyp bzučák\n");
            printf("\n");
            printf("🔄 OSTATNÍ:\n");
            printf("  on()                         - zapne motory\n");
            printf("  off()                        - vypne motory\n");
            printf("  stop()                       - okamžité zastavení\n");
            printf("  help()                       - tento help\n");
            printf("==============================\n");
            
        } else {
            printf("❌ Neznámý příkaz: %s\n", incomingPacket);
            printf("Napiš 'help()' pro seznam příkazů\n");
        }
    }
}
} // namespace rk
#pragma once

#include <WiFi.h>
#include <WiFiClient.h>
#include "robotka.h"

namespace rk {
    class Wifi {
    public:
        static void setupWiFiControl(const rkConfig& cfg);
        static bool handleWebClients();
        static void handleWebClients_terminal();
        
    private:
    };
};



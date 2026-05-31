#include <Arduino.h>
#include "robotka.h"

void setup() {
    rkConfig cfg;
    
    // Příklad nastavení vlastních pinů pro přední a zadní tlačítka:
    // Zde nastavíte na jakých GPIO pinech ESP32 fyzicky máte svá tlačítka napojena
    cfg.Button1 = 32; 
    cfg.Button2 = 33; 
    
    // Zapneme robota a předáme mu naší konfiguraci pinů
    rkSetup(cfg);

    printf("Robotka started!\n");
    rkLedBlue();

    // Počkáme 5 sekund na položení robota na zem před započetím pohybu
    delay(5000);

    // PŘÍKLAD 1: Jízda dopředu (front_buttons), dokud nenarazí vlastními tlačítky.
    // Používáme rkButton1() a rkButton2(), což jsou vestavěné funkce knihovny, 
    // které se automaticky dívají přímo na GPIO piny 32 a 33 definované výše v konfiguraci.
    front_buttons(40, []{ return rkButton1(); }, []{ return rkButton2(); });

    // Zastavíme s rozsvícenou zelenou LED a počkáme 2 sekundy po nárazu
    rkMotorsSetSpeed(0, 0);
    rkLedGreen();
    delay(2000);

    // PŘÍKLAD 2: Couvání (back_buttons) dokud nenarazí.
    // Pokud nemáme vzadu také další fyzické spínače na drátech, můžeme použít 
    // ta zabudovaná hardwarová tlačítka z řídící desky Robotky na levé a pravé straně.
    back_buttons(40, []{ return rkButtonLeft(); }, []{ return rkButtonRight(); });
    
    // Dokončili jsme pohyb.
    rkMotorsSetSpeed(0, 0);
    rkLedAll(true);
}

void loop() {
    // Smyčka zůstává prázdná, robot po splnění úlohy v setupu zastaví.
    delay(10);
}

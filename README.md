# RBCX-BEST 🤖 :::

## 🚀 Účel projektu

Cílem projektu **RBCX-BEST** je vylepšit funkčnost knihovny Robotka. Projekt přináší nové funkce a optimalizace, které zlepšují použitelnost a efektivitu knihovny. Všechny funkce jsou popsány v lib/RB3204-RBCX-Robotka-library-master/src/robotka.h !!!  ----> https://github.com/ZemanNz/RBCX-BEST/blob/main/lib/RB3204-RBCX-Robotka-library-master/src/robotka.h

![RBCX Front View](https://github.com/ZemanNz/OSEKANA_ROBOTKA_PROJEKT/blob/main/obrazky/Rbcx-front.png)

### Klíčové vlastnosti:

- **Uvolnění paměti:** Optimalizace knihovny snížila využití paměti z přibližně 60 % na 27 %, což zvyšuje efektivitu a dává nám prostor.
- **Desetinné řízení rychlosti:** Přidána možnost nastavit rychlost s desetinnou hodnotou, např. 50.5 místo pouze celých čísel (50 nebo 51). Díky tomu můžeme přesněji nastavovat přímý pohyb.
- **Integrace senzorů:** Nové funkce pro inicializaci a měření pomocí barevných a laserových senzorů. Podpora až dvou laserových senzorů (vyřešené měnění adresy).
- **Bezpečnost motorů:** Zaveden mechanismus, který zabraňuje zaseknutí programu při nedosažení cílové pozice. Nastaven timeout pro případ, že motor nedojede.
- **Přidání pohybových funkcí:** Funkce jako forward() nebo wall_following jsou vyzkousené a mají timeout. Reagují na zmeny nastavení robota v rkConfig() -- nap. polarita motorů, roztec kol, prevod motoru.

---

## 📂 Struktura projektu

📁 **RBCX-OSEKANY_ROBOT/**

- 📂 `.pio/`           – Výstup sestavení PlatformIO (lze kdykoliv smazat, při kompilaci se znovu vytvoří)
- 📂 `.vscode/`        – Nastavení VS Code  
- 📂 `examples/`       – Ukázkové příklady použití (Je jich tam opravdu dost --- > vsechny ozkousené)
- 📂 `include/`        – Vlastní hlavičkové soubory (např. `I2C`) -- ale o vetsinu se postara knihovna
- 📂 `lib/`            – Knihovny projektu (např. `RB3204-RBCX-Robotka-library-master/`)  
- 📂 `src/`            – Hlavní kód projektu (např. `main.cpp`)  
- 📂 `test/`           – Testovací skripty a jednotkové testy  
- 📜 `.gitignore`      – Ignorované soubory a složky pro Git  
- 📜 `gridui_mtime`    – Metadata pro GridUI  
- 📜 `ccache.py`       – Skript pro ccache  
- 📜 `partitions.csv`  – Konfigurace paměťových oddílů  
- 📜 `platformio.ini`  – Konfigurační soubor PlatformIO  
- 📜 `README.md`       – Dokumentace projektu  
- 📜 `to do.`          – To do list ---> opravdu se vyplatí

---

## ⚙️ Konfigurace (`rkConfig()`)

`rkConfig()` je součást knihovny Robotka a najdeš ji v souboru `robotka.h`. Umožňuje nakonfigurovat různé parametry hardware, například nastavení pinů pro motory, polaritu motorů, zrychlení motorů, maximální rychlost motorů a podobně.

### Ukázková konfigurace:

```cpp
rkConfig()
        : prevod_motoru(1979.3f) // pro 12v ==  41.62486f * 48.f, pro 6v == 1981.3f
        , left_wheel_diameter(62.2) // v mm
        , right_wheel_diameter(62) // v mm
        , roztec_kol(135.0) // v mm
        , konstanta_radius_vnejsi_kolo(1.064) // Korekční faktor pro vnější kolo při zatáčení
        , konstanta_radius_vnitrni_kolo(1.017f) // Korekční faktor pro vnitřní kolo při zatáčení
        , korekce_nedotacivosti_left(1.12f)// Korekce nedotáčivosti při otaceni na miste do leva
        , korekce_nedotacivosti_right(1.12f)// Korekce nedotáčivosti při otaceni na miste do prava
        , Button1(14)
        , Button2(35)
        , motor_id_left(1)
        , motor_id_right(4)
        , motor_max_power_pct(100)
        , motor_polarity_switch_left(false)
        , motor_polarity_switch_right(true)
        , motor_enable_failsafe(false)
        , motor_wheel_diameter(62)
        , motor_max_ticks_per_second(5200) // vyzkousite tak ze spustite funkci max_rychlost() a podle toho nastavite
        , motor_max_acceleration(50000)
        , stupid_servo_min(-1.65f)
        , stupid_servo_max(1.65f)
        , pocet_chytrych_serv(0)
        , enable_wifi_log(false)
        , enable_wifi_control_wasd(false)
        , enable_wifi_terminal(false)
        , wifi_ssid("robot1234")        // pro wasd a wifi_terminal je to jmeno wifi ktere robot vytvori!!! , pro logovani je to wifi ke ktere se pripoji ----> bacha to jmeno musi byt nejak dlouhy, jinak vam to nepujde prejmenovat
        , wifi_password("1234robot") {       // pro wasd a wifi_terminal je to heslo wifi ktere robot vytvori!!! , pro logovani je to wifi ke ktere se pripoji ----> bacha to heslo musi byt nejak dlouhy, jinak vam to nepujde prejmenovat
    }
```
**Varování**
- Vše je potrěba správně nastavit. Při špatném nastavení se robot může zaseknout, nebude reagovat. Např. pokud nemáte připojené ani jeno ch. servo
, ale máte nastavený pocet_chytrych_serv(2) --- na 2, tak se program bude snazit tyto serva inicializovat a nepustí vás dál.

## 🔧 Konfigurace PlatformIO (`platformio.ini`)

Soubor `platformio.ini` definuje prostředí a nastavení projektu. Obsahuje například:

- Verzi platformy (`platform = espressif32@~1.12.4`)
- Definici desky (`board = esp32dev`)
- Další volby jako `monitor_speed` a `upload_port`
- Knihovny uvedené pod klíčem `lib_deps` (sem se dávají knihovny, které chci, aby se při kompilaci stáhly) ---- mohu to nechat prázdné a knihovny přidat ručně do složky lib
- Pokud nepoužívám laserové senzory, mohu ("-DUSE_VL53L0X") odstranit, a uvolnit místo

```ini
; PlatformIO Project Configuration File
[env:esp32dev]
platform = espressif32@~1.12.4
board = esp32dev
framework = arduino
monitor_speed = 115200
upload_speed = 921600
board_build.partitions = partitions.csv
build_flags = -std=c++14 -DUSE_PRECOMPILED_LIBRARIES -DCCACHE -DUSE_VL53L0X
build_unflags = -std=gnu++11
monitor_filters = esp32_exception_decoder
extra_scripts = pre:ccache.py
lib_deps = 
        https://github.com/adafruit/Adafruit_TCS34725/archive/refs/tags/1.3.6.tar.gz
        SPI
        adafruit/Adafruit_VL53L0X @ ^1.2.4

```

## 🔧 Ovládání tlačítek a LED

**Tlačítka na desce:**  
- Button1, Button2, Button3, Button4 (Směry: Nahoru, Vlevo, Vpravo, Dolů)
- On, Off, Reset

Tato tlačítka lze také připojit externě k určeným pinům na desce (podle popisků na desce).

```cpp
if (rkButtonIsPressed(BTN_LEFT)) // pokud je tlačítko stisknuto
```

**LED indikátory:**  
Na desce jsou čtyři LED diody: Červená, Zelená, Modrá, Žlutá.  
Lze je ovládat pomocí:

```cpp
rkLedBlue(true);   // Zapnutí modré LED
rkLedBlue(false);  // Vypnutí modré LED
```

## 🔌 Připojení periférií

- **Motory:**
  - 4 konektory pro motory s enkodéry.

- **Ultrazvukové senzory:**
  - 4 piny označené jako U1–U4.

- **Serva:**
  - 4 konektory pro připojení serv.

### Tabulka pinů - pinout

!["Tabulka pinů - pinout"](https://github.com/ZemanNz/OSEKANA_ROBOTKA_PROJEKT/blob/main/obrazky/Tabulka_s_piny.png)

- podle tabulky si muzete zjistit, jake piny muzete vyuzit, jaké mají číslo a jak se jmenují na desce.
- tabulku si muzete sthnout : https://github.com/RoboticsBrno/RB3204-RBCX/blob/master/hw/pinout.xlsx

### Ukázka DPS v kicadu

!["Ukázka DPS v kicadu"](https://github.com/ZemanNz/OSEKANA_ROBOTKA_PROJEKT/blob/main/obrazky/Sn%C3%ADmek%20obrazovky%20z%202025-10-24%2021-50-48.png)

- pokud by vám nestacila tabulka můžete si stáhnout návrh DPS a tam zjistit vse co potrebujete.
- zde je znázorněna část, která ukazuje GPIO piny a UART -- pozor na deskách jsoou španné popisky --- TX a RX jsou správně tady v ukázce.
- pokud mate kicad. tak s muzete projekt stahnout zde : https://github.com/RoboticsBrno/RB3204-RBCX/blob/master/hw/RBCX.brd a https://github.com/RoboticsBrno/RB3204-RBCX/blob/master/hw/RBCX.sch ------ nasledne tyto dva soubory musite dat do slosky ktera se vam vygeneruje ---> /Stažené/RBCX

### Tlačítka:
 - Tlačítka lze připojit na IN1 a IN2 (analogové hodnoty, ale mohou se použít i jako digitální) 
 - Piny: IN1 = 36, IN2 = 39;, IN3 = 34 a IN4 = 35.
 - Piny GPIO27 a GPIO14.

- **GPIO piny -- Viz obrázky :**
  - GPIO27: Pin vzdálenější od středu desky. Viz obrázky.
  - GPIO14: Pin blíže ke středu desky. Viz obrázky.
  - GPIO25: Viz obrázky.
  - GPIO26: Viz obrázky.

!["GPIO piny - GPIO27, GPIO14"](https://github.com/ZemanNz/OSEKANA_ROBOTKA_PROJEKT/blob/main/obrazky/SENZORY-15-edit.jpg)
!["IN1 a IN2"](https://github.com/ZemanNz/OSEKANA_ROBOTKA_PROJEKT/blob/main/obrazky/SENZORY-25-edit.jpg)

- **Doporučení:**
  - Při připojování periférií dbejte na správné napájecí napětí (3.3V nebo 5V podle specifikace).
  - Pro další piny a podrobnosti konzultujte dokumentaci desky.


## 🔌 Zapojení a konfigurace pinů

### Zapojení I2C:
Sběrnice I2C umožňuje připojení více zařízení na stejné vodiče. V tomto projektu jsou pro komunikaci I2C použity následující piny:

- **SDA (datový vodič)** → GPIO21
- **SCL (hodinový vodič)** → GPIO22

Ujistěte se, že připojená I2C zařízení mají správnou adresu a odpovídající napájecí napětí (3.3V nebo 5V podle specifikace).

!["I2C-1 piny"](https://github.com/ZemanNz/OSEKANA_ROBOTKA_PROJEKT/blob/main/obrazky/popsane%20I2C%20piny.png)
- na obrazku jsou uvedene kde zapojit I2C-1.
- pro sda a scl jsou 2 dvojice pinů vzdalenějších od středu desky. další 2 dvojice jsou na +;
- v codu sda = 21 a scl = 22.

!["I2C-2 piny"](https://github.com/ZemanNz/OSEKANA_ROBOTKA_PROJEKT/blob/main/obrazky/I2C-2-.png)
- na obrazku jsou uvedene kde zapojit I2C-2.
- v codu sda = 14 a scl = 26.

## Práce s I2C

- RBCX deska podporuje 2 I2C sběrnice.
- Jedna I2C sběrnice je možná inicializovat na SDA(21) a SCL(22). To jsou piny (na desce oznaceny I2C rámečkem) , tedy piny D: SDA, C: SCL.
- Druhou I2C sběrnici lze inicializovat na SDA(14) a SCL(26). Jsou to piny v sekci UART, oznaceny: IO6, IO4.
- Na I2C sběrnici je muzeme zapojit senzory, které mají piny SDA a SCL, jako třeba TCS34725 nebo VL53L0X.
- Laserový senzor: Inicializace pomocí rk_laser_init(name, wire, sensor, pin, address).
- Barevný senzor: Inicializace pomocí rkColorSensorInit(name, wire, sensor).
- Měření vzdálenosti: Funkce rk_laser_measure(name) vrací vzdálenost v mm nebo -1 při chybě.
- RGB data: Funkce rkColorSensorGetRGB(name, &r, &g, &b) vrací true při úspěšném načtení.
- Skenování I2C: K dispozici funkce pro ověření připojení na sběrnicích Wire a Wire1.
- Správná inicializace: Senzory inicializujte ve funkci setup() před jejich použitím.
- Unikátní adresy: Laserové senzory potřebují unikátní adresy, aby nedošlo ke konfliktu (např. 0x30).
- Barevné senzory TCS34725 nepodporují zmenu adresy a mají neměnnou 0x29.
- Lserove senzory VL53L0X mají XSHUT pin, na ktery pokud pustime LOW dočasně vypneme.

## 🟦 Práce s UART

UART (Universal Asynchronous Receiver/Transmitter) je jednoduché rozhraní pro sériovou komunikaci mezi deskou a dalšími zařízeními (například jiným mikrokontrolérem, PC, nebo periferiemi). Na desce RBCX je UART2 vyveden na pinech GPIO16 (RX) a GPIO17 (TX).

### 📸 Zapojení UART na desce RBCX -- Popisky na desce jsou špatně!!!!

!["UART2_kicad"](https://github.com/ZemanNz/OSEKANA_ROBOTKA_PROJEKT/blob/main/obrazky/UART2_kicad.png)
- GND vedle je RX a vedle TX

![UART2 v realite](https://github.com/ZemanNz/OSEKANA_ROBOTKA_PROJEKT/blob/main/obrazky/zapojen%C3%AD_UART_rbcx_esp32.jpg)
- zapojení v realitě --> k druhému zařízení se vzdy pripojuje RX - rbcx na TX - druheho zarizeni a RX - druheho zarizeni na TX - rbcx
- Na obrázku je vidět připojení vodičů k RBCX desce na UART2(na desve ESP UART2, RX a TX piny vedle IO5)(v codu: rx:16, tx:17.

![TX na RX a na opak](https://github.com/ZemanNz/OSEKANA_ROBOTKA_PROJEKT/blob/main/obrazky/Sn%C3%ADmek%20obrazovky%20z%202025-10-24%2021-55-12.png)
-zapojeno uart mezi 2 esp32 --->vzdy rx na tx a naopak + spolecny GND!!!



### 📚 Knihovna pro příjem struktury přes UART

- **Inicializaci UART:**  
  Příklad použití je v examples/UART ---> je tu main.cpp pro rbcx a projekt pro esp32.

  Funkce `rkUartInit();` nastaví UART2 na rychlost 115200 baudů.

- **Příjem struktury:**  
  Funkce `rkUartReceive()` umožňuje přijímat libovolnou strukturu (například příkaz pro servo nebo motor) s jednoduchým framingem (každá zpráva začíná bajty 0xAA 0x55).  
  Funkci předáš odkaz na strukturu, kterou tato funkce při úspěšném příjmu automaticky naplní přijatými daty.  
  Funkce čeká na přijetí celé struktury, vypíše přijaté bajty na Serial monitor a vrátí `true`, pokud byla struktura úspěšně přijata.  
  Díky tomu můžeš s naplněnou strukturou dále pracovat ve svém kódu.

- **Odeslání struktury:**
  Funkce `rkUartSend()` umožňuje odesilat odesilat data pomoci odkazu na strukturu, kterou má poslat.

- **__attribute__((packed))**
  Toto umozňuje poslat co nejmene dat, jinak by se doplnilo do celyho byte.

#### Ukázka použití v kódu

```cpp
#include <Arduino.h>
#include "robotka.h"

// Nejjednodušší struktura
typedef struct __attribute__((packed)) {
    uint8_t servo_id;
    uint8_t position; // 0-255
} SimpleCommand;

SimpleCommand cmd = {1, 128}; // Servo ID 1, střední pozice

void setup() {
    rkConfig cfg;
    rkSetup(cfg);
    printf("Robotka started!\n");
    
    rkLedRed(true); // Turn on red LED
    rkLedBlue(true); // Turn on blue LED

    delay(2000); // Wait for 2 seconds
    rkUartInit();

    delay(100); // Short delay to ensure UART is initialized
    printf("posilam data...\n");
    rkUartSend(&cmd, sizeof(cmd));
    
}
int start_mil = millis();
void loop() {
    
    if(millis() - start_mil > 3000) {
        start_mil = millis();
        printf("posilam data...\n");
        rkUartSend(&cmd, sizeof(cmd));
    }
    if(rkUartReceive(&cmd, sizeof(cmd))) {
        // TADY PRACUJEME S PŘIJATÝMI DATY:
        
        // 1. Výpis na serial
        printf("Servo %d -> Position %d\n", cmd.servo_id, cmd.position);

    }
}
```

- **Stačí změnit strukturu podle potřeby.**
- Funkce je univerzální – můžeš přijímat jakýkoliv typ struktury.

---

## 🧭 Mpu Gyroskop (MPU6050)

Deska RBCX obsahuje integrovaný gyroskop a akcelerometr **MPU6050** připájený přímo na desce. Pokud senzor na vaší desce není osazen, všechny funkce budou vracet hodnotu `0.00`.

Pro snadnou práci s gyroskopem jsou v knihovně Robotka připraveny následující funkce:

### ⚙️ Inicializace a kalibrace
- `rkMpuInit()` – Inicializuje senzor a provede automatickou kalibraci (odstranění klidového driftu gyroskopu).  
  ⚠️ **DŮLEŽITÉ:** Během inicializace (která trvá cca 1,1 sekundy) **nesmí být robot v pohybu**, jinak se zkalibruje špatně a hodnoty budou samovolně ujíždět!

### 📊 Čtení hodnot (úhlů)
- `rkMpuGetAngleZ()` – Vrátí úhel natočení robota v **ose Z** (Yaw - rotace doleva/doprava) ve stupních.
- `rkMpuGetAngleX()` – Vrátí úhel náklonu v **ose X** (Pitch - předklon/záklon) ve stupních.
- `rkMpuGetAngleY()` – Vrátí úhel náklonu v **ose Y** (Roll - náklon do stran) ve stupních.

### 🔄 Resetování úhlů (vynulování)
- `rkMpuResetZ()` – Vynuluje úhel na ose Z. Ideální použít těsně před začátkem zatáčení, abyste mohli zatočit přesně o relativní úhel ze stávající pozice.
- `rkMpuResetX()` – Vynuluje aktuální úhel na ose X (nastaví aktuální náklon jako nulový bod).
- `rkMpuResetY()` – Vynuluje aktuální úhel na ose Y (nastaví aktuální náklon jako nulový bod).
- `rkMpuResetAll()` – Vynuluje úhly na všech třech osách naráz.

### 📸 Umístění senzoru na desce
![Mpu Gyroskop](https://github.com/ZemanNz/OSEKANA_ROBOTKA_PROJEKT/blob/main/obrazky/mpu.png)

## Práce s chytrými servy

!["obr.bur.konektor"](https://github.com/ZemanNz/OSEKANA_ROBOTKA_PROJEKT/blob/main/obrazky/konektor_ch.serv.jpg)
- pokud je vystupek nahoře, tak serial pin je v pravo smerem do serva
- stejně je to i u serv s jinym typem konektorů

!["obr.zapojenich.serv na desce"](https://github.com/ZemanNz/OSEKANA_ROBOTKA_PROJEKT/blob/main/obrazky/zapojeni_na_desce_ch_serv.jpg)
- tyto konektory pripajene na desce jsou pod napetim primo z baterie --> servo to zvládne

!["obr.pullup rezistor na iservo"](https://github.com/ZemanNz/OSEKANA_ROBOTKA_PROJEKT/blob/main/obrazky/pullup.jpg)
- pullup rezistor (10 KΩ, po namereni napajeneho to bude 4,4....KΩ ---> nejspise nejake paralerni zapojeni)je nutne napajet mezi 3.3V a iservo protože (nejspise kdyz se zrovna se servem nekomunikuje tak zadne napeti na iservu muze delat problemy)

* za sebe lze zapojit libovolny pocet ch. serv, ale musí mít spravne nastavený ID --> deska pripojena na servo id ==0 , na to napojeny servo id ==1 .......
- !!! Je notne v robotka.h nastavit spravne pocet chytrych serv---> pocet_chytrych_serv(2) ---> mam 2 chytry serva!!!
- priklad na pouziti ch. serv je v examples

#### Nastavení ID ch. serv
!["obr zapojeni GPIO14 pro nastaveni id"](https://github.com/ZemanNz/OSEKANA_ROBOTKA_PROJEKT/blob/main/obrazky/zapojeni_chy._serv.jpg)
- na obrázku vidíte správné zapojení pro nastavení ID serva
- napajeni serva je vzdy z 5V - treba z pinu pro normalni servo
- iservo pin pripojte na GPIO14 ---- to je GPIO -6 ---> pouziva se tez na I2C2
* pro nastaveni chytrych serv pouzijte tento kod: 
```cpp
#include <Arduino.h>
#include "SmartServoBus.hpp"

using namespace lx16a;

static int n = 0;
static SmartServoBus servoBus;

void setup() {
    // Servos on the bus must have sequential IDs, starting from 0 (not 1)!
    servoBus.begin(1, UART_NUM_2, GPIO_NUM_14);

    // Set servo Id (must be only one servo connected to the bus)
    servoBus.setId(0); //nastavení serva na ID = 0
    while (true) {
        printf("GetId: %d\n", servoBus.getId()); // servo bude vypisovat jaky je id
        delay(1000);
    }
    
}
void loop() { // pro zkouzku pohybu....

    servoBus.set(0, Angle::deg(0));

    delay(5000);

    servoBus.set(0,Angle::deg(240));
    delay(5000);
}

```

## 🟦 Práce se Serial monitorem

Tato knihovna umožňuje ovládat serva a motory přímo přes Serial monitor. Můžeš zadávat příkazy ve formě textových řetězců, které se následně zpracují a provedou odpovídající akce na robotu.

### ✨ Ovládání přes serial_monitor(příklad: `examples/ovladani_serial_monitor/ovladanii_pres_serial_monitor.cpp`)

* Dostupné příkazy:

=== POHYB ROBOTA ===
- forward(mm, speed)           - pohyb vpřed o zadanou vzdálenost (mm) a rychlost (%)
- forward_acc(mm, speed)       - pohyb vpřed s plynulým zrychlením a zpomalením
- backward(mm, speed)          - pohyb vzad o zadanou vzdálenost (mm) a rychlost (%)
- backward_acc(mm, speed)      - pohyb vzad s plynulým zrychlením a zpomalením
- turn_on_spot_left(angle, speed) - otočení na místě doleva o úhel (stupně) a rychlost (%)
- turn_on_spot_right(angle, speed) - otočení na místě doprava o úhel (stupně) a rychlost (%)
- radius_left(radius, angle, speed) - zatáčka doleva s poloměrem (mm), úhlem (stupně) a rychlost (%)
- radius_right(radius, angle, speed) - zatáčka doprava s poloměrem (mm), úhlem (stupně) a rychlost (%)
- back_buttons(speed)          - couvání, dokud nenarazí oběma tlačítky na zeď
- max_rychlost()               - změří maximální rychlost motorů (ticks za sekundu)
- stop()                       - okamžité zastavení motorů
- set_speed(left, right)       - nastaví rychlost motorů v % (-100 až 100)
- set_power(left, right)       - nastaví výkon motorů v % (-100 až 100)
 
=== SMART SERVA ===
- servo_init(id, [low, high])  - inicializace smart serva s ID, volitelně limity úhlu (low, high)
- servo_move(id, angle, [speed]) - rychlý pohyb serva na úhel (0-240°) s volitelnou rychlostí
- servo_soft_move(id, angle, [speed]) - plynulý pohyb serva s ochranou proti zaseknutí
- servo_position(id)           - přečte aktuální pozici smart serva

### 📝 Jak to funguje?

- Zadáš příkaz do Serial monitoru (např. `forward(1000,50)`).
- Program příkaz rozparsuje, zkontroluje parametry a zavolá odpovídající funkci.
- Výsledek (nebo případná chyba) se vypíše zpět do Serial monitoru.

---
**Díky této knihovně můžeš jednoduše testovat a ovládat robota bez nutnosti měnit kód – stačí zadávat příkazy přes Serial monitor!**



## 📡 Práce s WiFi
Knihovna Robotka nabízí několik možností pro komunikaci přes WiFi, včetně vzdáleného logování, ovládání pomocí WASD kláves a terminálu.

### 🔧 Konfigurace WiFi
WiFi funkce se konfigurují v `rkConfig()` struktuře:

```cpp
rkConfig cfg;
cfg.enable_wifi_log = false;        // Povolení WiFi logování
cfg.enable_wifi_control_wasd = false; // Povolení WASD ovládání  
cfg.enable_wifi_terminal = true;   // Povolení WiFi terminálu
cfg.wifi_ssid = "robotka1234";     // SSID WiFi sítě
cfg.wifi_password = "1234robotka"; // Heslo WiFi sítě

rkSetup(cfg);
```

### 📊 WiFi Logování
- priklady najdete v _librk_motors.cpp u pohybových funkcí
- robot musí být připojen na wifi
- po připojení se vypise ip adresa do s.m.

```cpp
void setup() {
    rkConfig cfg;
    cfg.enable_wifi_log = true;
    cfg.wifi_ssid = "moje_wifi";
    cfg.wifi_password = "moje_heslo";
    rkSetup(cfg);
}

void loop() {
    printf_wifi("Robot ujel: %.1f mm", 123.5);
    printf_wifi("Stav baterie: %d%%", rkBatteryPercent());
    
    handleWebClients(); // Nutné volat pravidelně
    delay(100);
}
```

### 🎮 WASD Ovládání
- priklad je v examples
- robot si vytvori vlastni wifi, na kterouse musite pripojit
- v tomto projektu naleznete slozku RBCX-controller ---> to je projektktery si spustite na svem zarizeni ---> musite zmenit ip adresu--- ta se zase vypise
- funkci muzeme na dalku ukoncit stisknutim P
- pokud je cfg.enable_wifi_control_wasd = true tak se wifi inicializuje sama

```cpp
void setup() {
    rkConfig cfg;
    cfg.enable_wifi_control_wasd = true;
    rkSetup(cfg);
}

void loop() {
    wifi_control_wasd(); // Blokující funkce ---> jde ukoncit pismenem P
}
```

### 💻 WiFi Terminál
- priklad je v examples
- robot si vytvori vlastni wifi, na kterouse musite pripojit
- v tomto projektu naleznete slozku RBCX-controller ---> to je projektktery si spustite na svem zarizeni ---> musite zmenit ip adresu--- ta se zase vypise
- pokud je cfg.enable_wifi_terminal = true tak se wifi inicializuje sama
- z funkce nelze vyjit --> je potreba restart robota

```cpp
void setup() {
    rkConfig cfg;
    cfg.enable_wifi_terminal = true;
    rkSetup(cfg);
}

void loop() {
    wifi_control_terminal(); // Blokující funkce
}
```

### 📝 Jak to funguje?
- **WASD ovládání:** Robot vytvoří WiFi síť, připojíte se a ovládáte pomocí kláves WASD/šipky  
- **WiFi terminál:** Připojíte se k robotovi a zadáváte textové příkazy jako v Serial monitoru  
- **WiFi logování:** Robot se připojí k vaší WiFi a logy můžete číst v prohlížeči na IP adrese

### Klávesy WASD ovládání:
- **W/↑** – pohyb vpřed (60% rychlost)  
- **S/↓** – pohyb vzad (60% rychlost)  
- **A/←** – otáčení doleva (20% rychlost)  
- **D/→** – otáčení doprava (20% rychlost)  
- **P** – ukončení ovládání  
- **L, K, J, H, G, M, N, B** – spuštění vlastních funkcí

### ⚠️ Důležité upozornění
- **Nelze kombinovat režimy:** Nelze současně používat WASD ovládání, WiFi terminál a WiFi logování – každý má jiný typ inicializace WiFi.  
- **Blokující funkce:** `wifi_control_wasd()` a `wifi_control_terminal()` jsou blokující – robot čeká na připojení.  
- **Pravidelné volání:** Pro WiFi logování musíte volat `handleWebClients()` v loopu.  
- **Vlastní funkce v WASD:** Chcete-li přidat vlastní funkce pro tlačítka `L, K, J, ...`, upravte soubor `lib/RB3204-RBCX-Robotka-library-master/src/wifi_control.cpp` v metodě `Wifi::handleWebClients()`.

### 🔧 Přizpůsobení WASD ovládání
Chcete-li přidat vlastní funkce pro tlačítka L, K, J, H, G, M, N, B, upravte v souboru `wifi_control.cpp`:

Díky WiFi funkcím můžeš ovládat robota na dálku, sledovat jeho logy a testovat chování bez nutnosti fyzického připojení kabelem!

# Startovací projekt pro robota (RBCX-BEST)

Tento repozitář slouží jako startovací bod pro tvého robota. Abys mohl psát svůj vlastní kód a zároveň dostávat naše aktualizace jádra knihovny, postupuj podle těchto kroků:

## 🚀 Instalace a použití

1️⃣ **Nainstalujte VS Code a PlatformIO**  
   - Stáhněte a nainstalujte [VS Code](https://code.visualstudio.com/).  
   - Přidejte rozšíření [PlatformIO IDE](https://platformio.org/install) a potřebné balíčky pro C++.  



## 1. Založení tvého robota (První spuštění)

1. Klikni na tlačítko **Fork** vpravo nahoře na této stránce. Tím se ti vytvoří tvá vlastní kopie repozitáře na tvém GitHub účtu. 
2. Pojmenuj si sve nove repo a potvrď. (repa budou provazane i pres jine jmeno)
2. Otevři si VS Code (nebo svůj terminál), přejdi do složky, kam chceš projekt uložit, a stáhni si **svůj** forknutý repozitář do počítače:

```bash
git clone [https://github.com/TVOJE_JMENO/RBCX-BEST.git]
```
*(Poznámka: Po stažení už je projekt plně propojený s Gitem, nemusíš nic inicializovat ani nastavovat.)*

3. Otevři si staženou složku ve VS Code / PlatformIO.

## 2. Jak programovat a ukládat postup

Své vlastní funkce a logiku robota piš **výhradně do souboru `src/main.cpp`** (případně si tvoř vlastní soubory ve složce `src`). 

Když chceš svou práci uložit a poslat k sobě na GitHub, použij klasický postup v terminálu:

```bash
# 1. Přidání všech změn ke zpracování
git add .

# 2. Vytvoření balíčku s popiskem toho, co jsi udělal
git commit -m "Popis toho, co jsem zrovna naprogramoval"

# 3. Odeslání na tvůj GitHub
git push
```

## 3. Jak stáhnout naše nejnovější aktualizace

Když vydáme vylepšení naší knihovny nebo opravíme chyby, můžeš si je bezpečně stáhnout do svého projektu. 

⚠️ **DŮLEŽITÉ:** Než to uděláš, ujisti se, že máš všechny své aktuální změny uložené u sebe na GitHubu (pomocí příkazů `git add`, `commit` a `push` viz sekce výše).

### Možnost A: Přes webové rozhraní GitHub (Sync Fork)

1. Jdi na stránku **svého** forknutého repozitáře na GitHubu.
2. Těsně pod zeleným tlačítkem *Code* uvidíš nápis *This branch is X commits behind ZemanNz:main*.
3. Klikni na tlačítko **Sync fork** a dej **Update branch**. (Tím se tvůj GitHub repozitář na internetu spojí s našimi novinkami).
4. Otevři svůj projekt ve VS Code a v terminálu si stáhni tyto spojené novinky z internetu k sobě do počítače:

```bash
git pull
```

Hotovo! Naše knihovny v projektu se zaktualizovaly a tvůj kód v `main.cpp` zůstal nedotčený.

### Možnost B: Synchronizace a řešení konfliktů přes Terminál (Sync přes Terminál)

Tento návod slouží k tomu, abyste si do svého forknutého projektu stáhli nejnovější opravy a knihovny z hlavního repozitáře (`RBCX-best`), ale **nepřišli přitom o svého rozepsaného robota v souboru `src/main.cpp`**.

#### 1. Prvotní nastavení (Stačí udělat pouze JEDNOU)
Pokud ještě nemáte ve svém lokálním Gitu propojený původní repozitář, otevřete terminál ve VS Kódu a přidejte si ho pod názvem `upstream`:
```bash
git remote add upstream https://github.com/RBCX-best/RBCX-BEST.git
```
*(Zda máte propojení správně, si můžete ověřit příkazem `git remote -v`)*

#### 2. Příkazy pro synchronizaci (Krok za krokem)

Kdykoliv vyjde v hlavním repozitáři aktualizace, zadejte v terminálu VS Kódu tyto příkazy::

##### Krok A: Stažení novinek do počítače
```bash
git fetch upstream
```
**Co to dělá:** Tento příkaz se podívá do hlavního repozitáře a stáhne z něj všechny nové úpravy (např. nové funkce v knihovnách nebo změny v `platformio.ini`) k vám do počítače. Zatím ale nic nemění ve vašich souborech, jen si je připraví do paměti.

##### Krok B: Sloučení kódů
```bash
git merge upstream/main
```
**Co to dělá:** Pokusí se spojit stažené novinky s vaším kódem.
- Pokud jste v daných souborech nic neměnili (např. v `robotka.cpp`), Git je aktualizuje sám automaticky.
- Pokud jste vy i hlavní repozitář měnili stejný soubor (typicky `src/main.cpp`), Git se zastaví, nahlásí `CONFLICT` a soubor uzamkne, dokud si nevyberete, kterou verzi chcete zachovat.

#### 3. Jak vyřešit konflikt ve VS Kódu (Potvrzení našich změn)

Když `git merge` nahlásí konflikt v `src/main.cpp`, soubor se ve VS Kódu zbarví do červena. Když ho otevřete, uvidíte nad konfliktním kódem svítit malé textové nabídky.

Pro zachování vašeho robota klikněte na:
- **Accept Current Change** (případně *Accept Ours*) = Tím Gitu natvrdo řeknete: „Ponechej tady můj kód, co mám v PC, a změny z hlavního repozitáře pro tento soubor ignoruj.“

Jakmile na to kliknete, gitové značky (`<<<<<<<` a `>>>>>>>`) zmizí. Soubor uložte (`Ctrl + S`) a dokončete proces v terminálu:
```bash
git add src/main.cpp
git commit -m "Aktualizace z upstreamu se zachovanim vlastniho main.cpp"
git push origin main
```
*(Příkaz `git push` odešle tuto čistou a spojenou verzi na váš osobní GitHub, takže i tam budete mít vše aktuální a bez chyb).*

## 4. Jak zachovat vlastní verzi README.md při synchronizaci (Pokročilé nastavení přes `.gitattributes`)

Pokud je v různých kopiích (forkách) repozitáře vyžadován odlišný obsah souboru `README.md` (např. specifická lokální dokumentace, obrázky či vlastní návody) a zároveň je nutné udržovat zbytek projektu synchronizovaný s hlavním repozitářem (`upstream`), lze využít soubor `.gitattributes` a strategii sloučení `merge=ours`.

Tento postup zajistí, že při provádění `git merge` nedojde k přepsání lokální verze souboru `README.md` změnami z jiného repozitáře a nevzniknou žádné konflikty.

### Jak to funguje? (Strategie sloučení "ours")

Do projektu se přidá konfigurační pravidlo, které Gitu přikáže: *„Při jakémkoliv slučování (merge) souboru `README.md` ignoruj změny přicházející z druhé větve/repozitáře a vždy zachovej lokální verzi souboru.“*

Aby toto chování fungovalo obousměrně a spolehlivě, musí toto nastavení provést všichni spolupracovníci ve svých lokálních repozitářích.

### Návod na nastavení krok za krokem:

#### Krok A: Vytvoření souboru `.gitattributes`
V kořenové složce projektu (tam, kde se nachází `.gitignore`) vytvořte soubor s názvem `.gitattributes` (pozor na tečku na začátku).
Do tohoto souboru vložte následující řádek:
```plaintext
README.md merge=ours
```
*Tento zápis určuje, že pro soubor README.md se má při sloučení použít strategie "ours".*

#### Krok B: Aktivace merge driveru v lokálním Gitu
Git standardně nepovoluje specifickou strategii sloučení pro jednotlivé soubory bez předchozí konfigurace. 
Otevřete terminál ve složce projektu a spusťte následující příkaz:
```bash
git config merge.ours.driver true
```
*(Tento příkaz musí provést každý spolupracovník na svém počítači).*

#### Krok C: Odeslání pravidla do repozitáře
Přidejte soubor `.gitattributes` do sledování verzí a odešlete jej na vzdálený server (GitHub):
```bash
git add .gitattributes
git commit -m "Nastaveni ochrany README pres gitattributes"
git push origin main
```

### Jak to funguje v praxi?

1. **Jeden z vývojářů** upraví `README.md` podle svých potřeb (např. přidá specifické grafické prvky či popis) a změny odešle na svůj GitHub (`git push`).
2. **Druhý vývojář** si v téže době spravuje v souboru `README.md` vlastní texty a návody.
3. Kdykoliv si kterýkoliv z vývojářů stáhne aktualizace kódu a knihoven z hlavního repozitáře (pomocí `git fetch upstream` a `git merge upstream/main`):
   - Git automaticky a bezpečně sloučí všechny ostatní soubory a knihovny (např. `robotka.cpp`).
   - Při pokusu o sloučení změn v `README.md` vyhodnotí pravidlo z `.gitattributes`, ignoruje příchozí změny z cizího repozitáře a ponechá v platnosti lokální verzi souboru.
   - Nedochází k žádným konfliktům a každý si zachová svou specifickou verzi dokumentace.

Tímto způsobem lze v různých kopiích repozitáře udržovat odlišný obsah souboru `README.md`, zatímco synchronizace funkčního kódu probíhá zcela automaticky.

## 5. Přejmenování projektu

- Pokud to prejmenujes na webu tak ok. Pokud na pocitaci tak musis aktualizovat cestu prikazem:

```bash
git remote set-url origin [https://github.com/TVOJE_JMENO/NOVY_NAZEV_ROBOTA.git]
```

Odteď budou tvé příkazy pro ukládání (`git push`) a stahování (`git pull`) bezpečně fungovat pod novým názvem.

3️⃣ **Sestavení a nahrání**  
   - Otevřete terminál pomocí "Ctrl + Shift + P" a spusťte příkazy PlatformIO. Ujistěte se, že máte správně vybraný COM port (např. COM4).  
   - Alternativně použijte následující příkazy:  

   ```bash
   pio run                 # Kompilace
   pio run --target upload # Nahrání na desku (vypněte Serial Monitor před nahráním !!!)
   pio run --target clean  # Vyčištění sestavení
   ```
![" "](https://github.com/ZemanNz/OSEKANA_ROBOTKA_PROJEKT/blob/main/obrazky/SharedScreenshot.jpg)
---

## ⚙️ Příkazy pro GitHub

Zde je několik užitečných příkazů pro správu repozitáře GitHub:

```bash
git init                 # Inicializace nového git repozitáře
git clone <url>          # Klonování existujícího repozitáře
git add .                # Přidání změn ke commitnutí
git commit -m "zpráva"  # Commit změn
git push                 # Odeslání na vzdálený repozitář
git pull                 # Stažení posledních změn z repozitáře
```


## Autor: (NZ)

- **Další projekt s RBCX:** 
- https://github.com/ZemanNz/RBCX-ROBOT-BRNO-2025.git
- https://github.com/ZemanNz/PUKY_2025.git
- https://github.com/ZemanNz/RBCX-BEARRESCUE-2025.git
